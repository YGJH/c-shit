// sudo apt-get update
// sudo apt-get install libboost-all-dev libssl-dev
// g++ -std=c++17 -O2 -I "C:\path\to\boost" o3Download.cpp -o o3Download -lboost_system -lssl -lcrypto -lpthread
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <string>
#include <cstdlib>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = net::ssl;               // from <boost/asio/ssl.hpp>
using tcp = net::ip::tcp;

// Performs a HEAD request over HTTPS to get the Content-Length.
std::size_t get_content_length_https(const std::string& host, const std::string& port, const std::string& target, ssl::context& ctx)
{
    net::io_context ioc;
    // Create an SSL stream (socket wrapped with SSL)
    ssl::stream<tcp::socket> stream(ioc, ctx);
    
    // Resolve the host and connect.
    tcp::resolver resolver(ioc);
    auto const results = resolver.resolve(host, port);
    net::connect(stream.next_layer(), results.begin(), results.end());
    stream.handshake(ssl::stream_base::client);

    // Build and send the HEAD request.
    http::request<http::empty_body> req{http::verb::head, target, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    http::write(stream, req);

    // Read the response.
    beast::flat_buffer buffer;
    http::response<http::empty_body> res;
    http::read(stream, buffer, res);
    
    // Shutdown the SSL connection.
    boost::system::error_code ec;
    stream.shutdown(ec);
    if(ec && ec != net::error::eof)
        std::cerr << "Shutdown error: " << ec.message() << std::endl;
    
    // Extract Content-Length header.
    auto cl = res[http::field::content_length];
    return cl.empty() ? 0 : std::stoull(std::string(cl));
}

// Downloads a segment (range) of the file via HTTPS and writes it to output_filename.
void download_segment_https(const std::string& host, const std::string& port, const std::string& target, 
                      std::size_t start, std::size_t end, const std::string& output_filename, ssl::context& ctx)
{
    try {
        net::io_context ioc;
        ssl::stream<tcp::socket> stream(ioc, ctx);
        tcp::resolver resolver(ioc);
        auto const results = resolver.resolve(host, port);
        net::connect(stream.next_layer(), results.begin(), results.end());
        stream.handshake(ssl::stream_base::client);

        // Build the GET request with a Range header.
        http::request<http::empty_body> req{http::verb::get, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::range, "bytes=" + std::to_string(start) + "-" + std::to_string(end));
        http::write(stream, req);

        // Read the response into a vector body.
        beast::flat_buffer buffer;
        http::response<http::vector_body<char>> res;
        http::read(stream, buffer, res);

        boost::system::error_code ec;
        stream.shutdown(ec);
        if(ec && ec != net::error::eof)
            std::cerr << "Shutdown error: " << ec.message() << std::endl;
        
        // Write the downloaded segment to a file.
        std::ofstream outfile(output_filename, std::ios::binary);
        outfile.write(res.body().data(), res.body().size());
        outfile.close();
    } catch(std::exception& e) {
        std::cerr << "Error in segment download: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[])
{
    if(argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <host> <target> <num_segments> <final_filename>\n";
        std::cerr << "Example: " << argv[0] << " github.com /USERNAME/REPOSITORY/archive/refs/heads/master.zip 4 final.zip\n";
        return EXIT_FAILURE;
    }
    std::string host = argv[1];
    std::string target = argv[2];
    int num_segments = std::atoi(argv[3]);
    std::string final_filename = argv[4];
    if(num_segments < 1)
        num_segments = 1;
    
    // For HTTPS use port 443.
    std::string port = "443";

    // Set up an SSL context.
    ssl::context ctx(ssl::context::sslv23_client);
    ctx.set_default_verify_paths();

    // Get the total size via a HEAD request.
    std::size_t total_size = get_content_length_https(host, port, target, ctx);
    if(total_size == 0) {
        std::cerr << "Failed to obtain Content-Length." << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Total file size: " << total_size << " bytes" << std::endl;

    // Determine each segment's size.
    std::size_t segment_size = total_size / num_segments;
    std::vector<std::thread> threads;
    std::vector<std::string> segment_files;

    for (int i = 0; i < num_segments; ++i) {
        std::size_t start = i * segment_size;
        std::size_t end = (i == num_segments - 1) ? total_size - 1 : (start + segment_size - 1);
        std::string seg_file = "segment_" + std::to_string(i) + ".bin";
        segment_files.push_back(seg_file);
        threads.emplace_back(download_segment_https, host, port, target, start, end, seg_file, std::ref(ctx));
    }

    // Wait for all download threads to finish.
    for(auto& t: threads)
        t.join();

    // Combine all segment files into the final file.
    std::ofstream outfile(final_filename, std::ios::binary);
    for(const auto& seg_file : segment_files) {
        std::ifstream infile(seg_file, std::ios::binary);
        outfile << infile.rdbuf();
        infile.close();
        std::remove(seg_file.c_str());
    }
    outfile.close();

    std::cout << "Download complete: " << final_filename << std::endl;
    return EXIT_SUCCESS;
}
