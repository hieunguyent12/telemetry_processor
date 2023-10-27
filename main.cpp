#include <iostream>
#include <string>
#include "sockpp/tcp_acceptor.h"
#include "sockpp/stream_socket.h"
#include "SensorData.h"
#include "SensorRules.h"
#include "cparse/shunting-yard.h"

using namespace std;

// Create a file stream for output
ofstream outputFile("output.txt", ios::app);

// TODO: call this function on the data
SensorData handleExpression(SensorData &data, SensorRules &rules) {
    cparse::calculator c1(rules.expr); // Create calculator based on expression
    cparse::TokenMap vars;
    vars["value"] = data.value;
    vars["previous_value"] = rules.prevValue;
    rules.prevValue = data.value; // Set new previous value

    cparse::packToken temp;
    temp = c1.eval(vars);
    int outputValue = temp.asInt(); // Store calculator output

    time_t currTimeTemp = std::time(nullptr); // Get current time
    int currTime = static_cast<int>(currTimeTemp);

    SensorData output(currTime, rules.labelOut, rules.unitsOut, outputValue); // Create output sensor data object
    return output;
}

std::string bufferToString(char* buffer, int bufflen)
{
    std::string ret(buffer, bufflen);

    return ret;
}

void process_data(sockpp::tcp_socket sock) {
    char buf[512];
    fd_set readfds;
    int fd_max;
    struct timeval tv;


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
            if (FD_ISSET(sock.handle(), &readfds)) {
                cout << "there is new data available!!" << endl;

                // if sock.read(...) returns 0, that means the client has disconnected
                if (sock.read(buf, sizeof(buf)) == 0) {
                    break;
                }

                // TODO: figure out a more efficient way to parse the buffer

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

                cout << "Data: " << data_string << endl;
                cout << "Length: " << length << endl;

                 // Append the processed data to the output file
                outputFile << data_string << ",\n";
                outputFile.flush(); // Ensure data is written to the file

            }
        }
    }

    cout << "Connection closed from " << sock.peer_address() << endl;
}

int main(int argc, char* argv[]) {
    cout << "Telemetry Processor 🚀" << endl;

    in_port_t port = 9000;
    sockpp::initialize();
    sockpp::tcp_acceptor acc(port);

    // TODO: put socket data into processing queue
    vector<int> processing_queue;

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