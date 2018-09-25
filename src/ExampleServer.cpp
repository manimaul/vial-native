#include <iostream>
#include "wkhttp/Config.h"
#include "wkhttp/HttpRequest.h"

#include "wkhttp/HttpRequest.h"
#include "wkhttp/HttpResponse.h"
#include "wkhttp/HttpServer.h"
#include <glog/logging.h>

int main(int argc, char *argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);
    google::InstallFailureSignalHandler();

    auto ipAddress = "0.0.0.0";
    auto port = 8080;

    VLOG(0) << "starting example server listening on: " << ipAddress << ":" << port;
    wk::HttpServer(
            wk::Config()
                    .setIpAddress(ipAddress)
                    .setPort(port)
                    .setIdleTimeoutSeconds(60))
            .addRoute("/foo", wk::HttpMethod::Post, [](wk::HttpRequest &request) {
                wk::HttpResponse response = wk::HttpResponse(wk::HttpStatus::Ok)
                        .addHeader("hi", "there")
                        .setBody("hello path POST foo");
                return response;
            })
            .addRoute("/foo", wk::HttpMethod::Get, [](wk::HttpRequest &request) {
                wk::HttpResponse response = wk::HttpResponse(wk::HttpStatus::Ok)
                        .addHeader("hi", "there")
                        .setBody("hello path GET foo");
                return response;
            })
            .addRoute("/baz", wk::HttpMethod::Get, [](wk::HttpRequest &request) {
                wk::HttpResponse response = wk::HttpResponse(wk::HttpStatus::Ok)
                        .addHeader("hi", "there")
                        .setBody("hello path GET baz");
                return response;
            })
            .listenAndServe();
}

