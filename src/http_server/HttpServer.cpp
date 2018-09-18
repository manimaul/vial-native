
#include "HttpServer.h"

#include <folly/Memory.h>
#include <folly/io/async/EventBaseManager.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>
#include <proxygen/httpserver/ResponseBuilder.h>

class RequestHandlerWrapper : public proxygen::RequestHandler {

public:

    RequestHandlerWrapper(wk::Handler &handler) : handler(handler) {}

    void onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override {
        // todo: capture request headers
    }

    void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override {
        //todo: capture request body
    }

    void onEOM() noexcept override {
        auto request = wk::HttpRequest();
        wk::HttpResponse response = handler(request);
        proxygen::ResponseBuilder(downstream_)
                .status(200, "OK")
                .header("hello", "world")
                .body(folly::IOBuf::create(0))
                .sendWithEOM();
    }

    void onUpgrade(proxygen::UpgradeProtocol proto) noexcept override {
        // handler doesn't support upgrades
    }

    void requestComplete() noexcept override {
        delete this;
    }

    void onError(proxygen::ProxygenError err) noexcept override {
        delete this;
    }

    wk::Handler handler;
};

class RequestHandlerFactory : public proxygen::RequestHandlerFactory {
public:
    RequestHandlerFactory(std::unordered_map<std::string, wk::Handler> &handlers) : handlers(handlers) {}

public:
    void onServerStart(folly::EventBase * /*evb*/) noexcept override {
    }

    void onServerStop() noexcept override {
    }

    proxygen::RequestHandler *onRequest(proxygen::RequestHandler *, proxygen::HTTPMessage *message) noexcept override {
        std::string path = message->getPath();
        auto got = handlers.find(path);
        if (got == handlers.end()) {
            wk::Handler handler = [](wk::HttpRequest &request) {
                return wk::HttpResponse(wk::HttpStatus::NotFound);
            };
            return new RequestHandlerWrapper(handler);
        } else {
            return new RequestHandlerWrapper(got->second);
        }
    }

private:
    std::unordered_map<std::string, wk::Handler> handlers;
};

wk::HttpServer::HttpServer(wk::Config const &config) : config(config), handlers() {}

void wk::HttpServer::listenAndServer() {
    std::vector<proxygen::HTTPServer::IPConfig> IPs = {
            {folly::SocketAddress(config.getIpAddress(), config.getPort(), true), proxygen::HTTPServer::Protocol::HTTP},
            {folly::SocketAddress(config.getIpAddress(), 11001,
                                  true),                                          proxygen::HTTPServer::Protocol::HTTP2},
    };

    proxygen::HTTPServerOptions options;
    options.threads = static_cast<size_t>(sysconf(_SC_NPROCESSORS_ONLN));
    options.idleTimeout = std::chrono::seconds(config.getIdleTimeoutSeconds());
    options.shutdownOn = {SIGINT, SIGTERM};
    options.enableContentCompression = false;
    options.handlerFactories = proxygen::RequestHandlerChain()
            .addThen<RequestHandlerFactory>(handlers)
            .build();
    options.h2cEnabled = true;

    proxygen::HTTPServer server(std::move(options));
    server.bind(IPs);

    // Start HTTPServer mainloop in a separate thread
    std::thread t([&]() {
        server.start();
    });

    t.join();
}

static std::shared_ptr<wk::Handler> makeShared(wk::Handler handler) {
    return std::make_shared<wk::Handler>(handler);
}

wk::HttpServer &wk::HttpServer::addRoute(std::string const &routePattern,
                                         HttpMethod const &method,
                                         Handler handler) {
    handlers.emplace(routePattern, handler);
    return *this;
}
