#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <map>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>

class HttpRequest {
public:
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;

    HttpRequest(const std::string& raw_request) {
        parse(raw_request);
    }

private:
    void parse(const std::string& raw_request) {
        std::vector<std::string> lines;
        std::string current_line;
        std::stringstream ss(raw_request);

        while (std::getline(ss, current_line)) {
            // Windows/HTTP uses \r\n, Unix uses \n. Let's handle both
            if (current_line.back() == '\r') {
                current_line.pop_back();
            }
            lines.push_back(current_line);
        }
        
        if (lines.empty()) return;

        // Request Line
        std::stringstream request_line_ss(lines[0]);
        request_line_ss >> method >> path;

        // Headers
        size_t i = 1;
        while (i < lines.size() && !lines[i].empty()) {
            size_t separator_pos = lines[i].find(": ");
            if (separator_pos != std::string::npos) {
                std::string key = lines[i].substr(0, separator_pos);
                std::string value = lines[i].substr(separator_pos + 2);
                headers[key] = value;
            }
            i++;
        }

        // Body
        if (i < lines.size()) {
            body = lines[i+1];
        }
    }
};

class HttpResponse {
public:
    int status_code;
    std::string body;
    std::map<std::string, std::string> headers;

    HttpResponse(int code, const std::string& content) : status_code(code), body(content) {
        headers["Content-Type"] = "text/html";
        headers["Content-Length"] = std::to_string(body.length());
    }

    std::string to_string() {
        std::string response = "HTTP/1.1 " + std::to_string(status_code) + " " + get_status_message() + "\r\n";
        for (const auto& header : headers) {
            response += header.first + ": " + header.second + "\r\n";
        }
        response += "\r\n";
        response += body;
        return response;
    }

private:
    std::string get_status_message() {
        switch (status_code) {
            case 200: return "OK";
            case 404: return "Not Found";
            case 405: return "Method Not Allowed";
            default: return "Unknown";
        }
    }
};

class HttpServer {
public:
    HttpServer(const std::string& host, int port, const std::string& web_dir)
        : host_(host), port_(port), web_dir_(web_dir) {}

    void start() {
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == 0) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }

        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);

        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }

        if (listen(server_fd, 3) < 0) {
            perror("listen");
            exit(EXIT_FAILURE);
        }

        std::cout << "Listening on " << host_ << ":" << port_ << std::endl;

        while (true) {
            int new_socket;
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&address)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            char buffer[1024] = {0};
            read(new_socket, buffer, 1024);
            HttpRequest request(buffer);

            HttpResponse response = handle_request(request);

            std::string response_str = response.to_string();
            send(new_socket, response_str.c_str(), response_str.length(), 0);
            close(new_socket);
        }
    }

private:
    HttpResponse handle_request(const HttpRequest& request) {
        if (request.method == "GET") {
            return handle_get_request(request);
        } else {
            return HttpResponse(405, "<h1>405 Method Not Allowed</h1>");
        }
    }

    HttpResponse handle_get_request(const HttpRequest& request) {
        std::string path = request.path;
        if (path == "/") {
            path = "/index.html";
        }

        std::string file_path = web_dir_ + path;
        std::ifstream file(file_path);

        if (file.good()) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            return HttpResponse(200, content);
        }
        else {
            return HttpResponse(404, "<h1>404 Not Found</h1>");
        }
    }

    std::string host_;
    int port_;
    std::string web_dir_;
};

int main() {
    HttpServer server("127.0.0.1", 8008, "../www");
    server.start();
    return 0;
}
