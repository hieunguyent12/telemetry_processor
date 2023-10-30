#include <iostream>
#include <string>
#include "sockpp/tcp_acceptor.h"
#include "sockpp/stream_socket.h"
#include "SensorData.h"
#include "SensorRules.h"
#include "cparse/shunting-yard.h"
#include <fstream> // Add this include for file operations
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using namespace std;

// Create a file stream for output
ofstream outputFile("output.txt", ios::app);

SensorData applyProcessingRule(SensorData &data, SensorRules &rules) {
    cparse::calculator c1(rules.expr); // Create calculator based on expression
    cparse::TokenMap vars;
    vars["value"] = data.value;
    vars["previous_value"] = rules.prevValue;
    rules.prevValue = data.value; // Set new previous value

    cparse::packToken temp;
    temp = c1.eval(vars);
    int outputValue = temp.asInt(); // Store calculator output

    time_t currTimeTemp = std::time(nullptr); // Get current time

    SensorData output {
            currTimeTemp,
            rules.labelOut,
            rules.unitsOut,
            outputValue
    };

    return output;
}

std::string bufferToString(char* buffer, int bufflen)
{
    std::string ret(buffer, bufflen);

    return ret;
}

// this function is needed to convert a json string to SensorData
void from_json(const json& j, SensorData& s) {
    j.at("t").get_to(s.time);
    j.at("l").get_to(s.label);
    j.at("u").get_to(s.units);
    j.at("v").get_to(s.value);
}

void to_json(json& j, const SensorData& s) {
    j = json{{"t", s.time}, {"l", s.label}, {"u", s.units}, {"v", s.value}};
}


void process_data(sockpp::tcp_socket sock) {
    char buf[512];
    fd_set readfds;
    int fd_max;
    struct timeval tv;
    vector<SensorData> queue;


    // This while loop will keep the socket alive as long as the client is still connected
    // the "select" function will notify us when new data is available on the socket for us to consume
    while (true) {
        FD_ZERO(&readfds);
        FD_SET(sock.handle(), &readfds);

        fd_max = sock.handle();

        tv.tv_sec = 10;
        tv.tv_usec = 500000;
        int rv = select(fd_max + 1, &readfds, NULL, NULL, &tv);

        if (rv == -1) {
            perror("select");
        } else if (rv == 0) {
            // timeout
            printf("Timeout occurred! No data after 10.5 seconds.\n");
        }
        else {
            cout << FD_ISSET(sock.handle(), &readfds) << endl;
            if (FD_ISSET(sock.handle(), &readfds)) {
                cout << "there is new data available!!" << endl;

                // if sock.read(...) returns 0, that means the client has disconnected
                if (sock.read(buf, sizeof(buf)) == 0) {
                    break;
                }

                int length = int((unsigned char)(buf[0]) << 24 |
                                 (unsigned char)(buf[1]) << 16 |
                                 (unsigned char)(buf[2]) << 8 |
                                 (unsigned char)(buf[3]));

                vector<char> data;

                // I don't really know why we have to add 3 here, but it works
                char new_data[length + 3];

                // starting from index 4, we get all elements from the buffer until we reach the `length`
                for (int i = 4; i <= length + 3; i++) {
                   data.push_back(buf[i]);
                }

                std::copy(data.begin(), data.end(), new_data);

                string data_string = bufferToString(new_data, sizeof(new_data));

                data_string = data_string.substr(0, data_string.size() - 3);


                if (data_string == "START") {
                    unsigned char res[3] = "OK";
                    int32_t length = sizeof(res);
                    vector<unsigned char> res_buffer;

                    res_buffer.push_back(length >> 24);
                    res_buffer.push_back(length >> 16);
                    res_buffer.push_back(length >>  8);
                    res_buffer.push_back(length);

                    for (int i = 0; i < length; i++) {
                        res_buffer.push_back(res[i]);
                    }

                    unsigned char* a = res_buffer.data();
                    sock.write_n(a, sizeof(a));
                } else {
                    cout << "Data: " << data_string << endl;
                    cout << "Length: " << length << endl;

                    json json_data = json::parse(data_string);
                    auto sensor_data = json_data.template get<SensorData>();
                    queue.push_back(sensor_data);
                }
            } else {
                // TODO: process data in the queue while waiting for new data?
            }
        }
    }

    // Example sensor rule to apply on data
    SensorRules r("PSI to Bars", "sensor:IPT", "PSI", "value * 0.0689476", "sensor:IPT", "BARS");

    // process the data in the queue, if there are any
    for (int i = 0; i < size(queue); i++) {
        SensorData s = queue[i];

        SensorData result = applyProcessingRule(s, r);

        json result_string = result;

        outputFile << result_string << ",\n";
        outputFile.flush(); // Ensure data is written to the file
    }
    // Append the processed data to the output file
    cout << "Connection closed from " << sock.peer_address() << endl;
}

int main(int argc, char* argv[]) {
    cout << "Telemetry Processor 🚀" << endl;

    in_port_t port = 9000;
    sockpp::initialize();
    sockpp::tcp_acceptor acc(port);


    if (!acc) {
        cerr << "Error creating server: " << acc.last_error_str() << endl;
        return 1;
    }

    cout << "Listening on port " << port << "..." << endl;

    while (true) {
        sockpp::inet_address peer;
        sockpp::tcp_socket sock = acc.accept(&peer);

        cout << "Received a connection request from " << peer << endl;

        if (!sock) {
            cerr << "Error accepting incoming connection: "
                 << acc.last_error_str() << endl;
        }
        else {
            process_data(std::move(sock));
        }
    }
}