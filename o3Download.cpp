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
    boost::system::error_code ec;  // 在函式開頭宣告 ec
    net::io_context ioc;
    ssl::stream<tcp::socket> stream(ioc, ctx);
    std::size_t file_size = 0;

    std::cout << "正在連接到 " << host << target << std::endl;

    tcp::resolver resolver(ioc);
    boost::system::error_code resolve_ec;
    auto const results = resolver.resolve(host, port, resolve_ec);
    if (resolve_ec) {
        std::cerr << "解析 host 失敗: " << resolve_ec.message() << std::endl;
        return 0;
    }

    boost::system::error_code connect_ec;
    net::connect(stream.next_layer(), results.begin(), results.end(), connect_ec);
    if (connect_ec) {
        std::cerr << "連線失敗: " << connect_ec.message() << std::endl;
        return 0;
    }
    
    boost::system::error_code handshake_ec;
    stream.handshake(ssl::stream_base::client, handshake_ec);
    if(handshake_ec && handshake_ec != boost::asio::ssl::error::stream_truncated) {
        std::cerr << "Handshake 失敗: " << handshake_ec.message() << std::endl;
        return 0;
    }
    
    http::request<http::empty_body> req{http::verb::head, target, 11};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    
    boost::system::error_code write_ec;
    http::write(stream, req, write_ec);
    if (write_ec) {
        std::cerr << "寫入請求失敗: " << write_ec.message() << std::endl;
        return 0;
    }
    
    beast::flat_buffer buffer;
    http::response<http::empty_body> res;
    boost::system::error_code read_ec;
    http::read(stream, buffer, res, read_ec);
    if(read_ec && read_ec != boost::asio::ssl::error::stream_truncated) {
        std::cerr << "讀取回應失敗: " << read_ec.message() << std::endl;
    }
    
    if(res.result() != http::status::ok) {
        std::cerr << "HEAD 請求失敗，狀態碼: " << static_cast<int>(res.result()) << std::endl;
        std::cerr << "嘗試改用 GET 請求..." << std::endl;
        stream.shutdown(ec);  // 直接使用 ec
    } else {
        auto cl = res[http::field::content_length];
        if (!cl.empty()) {
            try {
                file_size = std::stoull(std::string(cl));
                std::cout << "透過 HEAD 取得檔案大小: " << file_size << " bytes" << std::endl;
                stream.shutdown(ec);  // 直接使用 ec
                return file_size;
            } catch (const std::exception& e) {
                std::cerr << "轉換 Content-Length 失敗: " << e.what() << std::endl;
                stream.shutdown(ec);  // 直接使用 ec
            }
        } else {
            std::cerr << "回應中沒有 Content-Length 標頭" << std::endl;
        }
    }
    
    std::cout << "嘗試使用 Range 請求取得檔案大小..." << std::endl;
    // Fallback: 如果 HEAD 未取到檔案大小，透過 GET 取 "Content-Range" 標頭。
    {
        net::io_context ioc2;
        ssl::stream<tcp::socket> stream2(ioc2, ctx);
        
        tcp::resolver resolver2(ioc2);
        boost::system::error_code resolve_ec2;
        auto const results2 = resolver2.resolve(host, port, resolve_ec2);
        if (resolve_ec2) {
            std::cerr << "解析 host 失敗 (Range 請求): " << resolve_ec2.message() << std::endl;
            stream.shutdown(ec);
            return 0;
        }

        boost::system::error_code connect_ec2;
        net::connect(stream2.next_layer(), results2.begin(), results2.end(), connect_ec2);
        if (connect_ec2) {
            std::cerr << "連線失敗 (Range 請求): " << connect_ec2.message() << std::endl;
            stream.shutdown(ec);
            return 0;
        }
        
        boost::system::error_code handshake_ec2;
        stream2.handshake(ssl::stream_base::client, handshake_ec2);
        if(handshake_ec2 && handshake_ec2 != boost::asio::ssl::error::stream_truncated) {
            std::cerr << "Handshake 失敗 (Range 請求): " << handshake_ec2.message() << std::endl;
            stream.shutdown(ec);
            return 0;
        }
        
        http::request<http::empty_body> req2{http::verb::get, target, 11};
        req2.set(http::field::host, host);
        req2.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req2.set(http::field::range, "bytes=0-0");
        req2.set(http::field::accept, "*/*");  // 加入這行
        
        boost::system::error_code write_ec2;
        http::write(stream2, req2, write_ec2);
         if (write_ec2) {
            std::cerr << "寫入請求失敗 (Range 請求): " << write_ec2.message() << std::endl;
            stream.shutdown(ec);
            return 0;
        }
        
        beast::flat_buffer buffer2;
        http::response<http::empty_body> res2;
        boost::system::error_code read_ec2;
        http::read(stream2, buffer2, res2, read_ec2);
        if (read_ec2 && read_ec2 != boost::asio::ssl::error::stream_truncated) {
            std::cerr << "讀取回應失敗 (Range 請求): " << read_ec2.message() << std::endl;
            std::cerr << "回應狀態碼: " << static_cast<int>(res2.result()) << std::endl;
            stream.shutdown(ec);
            return 0;
        }
        
        std::cout << "GET Range 請求回應狀態碼: " << static_cast<int>(res2.result()) << std::endl;

        auto cr = res2[http::field::content_range];
        if(!cr.empty()){
            std::cout << "收到 Content-Range 標頭: " << cr << std::endl;
            // 解析格式 "bytes 0-0/12345"
            std::string cr_str(cr);
            auto slash = cr_str.find('/');
            if (slash != std::string::npos) {
                std::string total_str = cr_str.substr(slash+1);
                try {
                    file_size = std::stoull(total_str);
                    std::cout << "透過 Range 請求取得檔案大小: " << file_size << " bytes" << std::endl;
                } catch(...) {
                    std::cerr << "解析 Content-Range 失敗" << std::endl;
                }
            }
        } else {
            std::cerr << "未取得 Content-Range 標頭" << std::endl;
        }
        stream2.shutdown(ec);
    }
    stream.shutdown(ec);
    return file_size;
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
        
        boost::system::error_code ec;
        stream.handshake(ssl::stream_base::client, ec);
        if(ec && ec != boost::asio::ssl::error::stream_truncated) {
            std::cerr << "Handshake error: " << ec.message() << std::endl;
            return;
        }
        
        http::request<http::empty_body> req{http::verb::get, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::range, "bytes=" + std::to_string(start) + "-" + std::to_string(end));
        http::write(stream, req);
        
        beast::flat_buffer buffer;
        http::response<http::vector_body<char>> res;
        boost::system::error_code read_ec;
        http::read(stream, buffer, res, read_ec);
        if(read_ec && read_ec != boost::asio::ssl::error::stream_truncated) {
            std::cerr << "Read error: " << read_ec.message() << std::endl;
            return;
        }
        
        stream.shutdown(ec);
        if(ec == boost::asio::ssl::error::stream_truncated)
            ec.clear();
        if(ec && ec != net::error::eof)
            std::cerr << "Shutdown error: " << ec.message() << std::endl;
        
        std::ofstream outfile(output_filename, std::ios::binary);
        outfile.write(res.body().data(), res.body().size());
        outfile.close();
    } catch(std::exception& e) {
        std::cerr << "Error in segment download: " << e.what() << std::endl;
    }
}

// 新增 URL 解析函數
struct ParsedUrl {
    std::string host;
    std::string target;
};

ParsedUrl parse_url(const std::string& url) {
    ParsedUrl result;
    
    // 移除 "https://" 或 "http://"
    size_t start = 0;
    if (url.substr(0, 8) == "https://") {
        start = 8;
    } else if (url.substr(0, 7) == "http://") {
        start = 7;
    }
    
    // 找到第一個 '/'
    size_t pos = url.find('/', start);
    if (pos != std::string::npos) {
        result.host = url.substr(start, pos - start);
        result.target = url.substr(pos);
    } else {
        result.host = url.substr(start);
        result.target = "/";
    }
    
    return result;
}

// 修改 main 函數
int main(int argc, char* argv[])
{
    if(argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <url> [output_filename]\n";
        std::cerr << "Example: " << argv[0] << " https://.../file.zip myfile.zip\n";
        return EXIT_FAILURE;
    }

    // 解析 URL
    std::string url = argv[1];
    
    // 取得輸出檔名（可藉由第二個參數指定）
    std::string final_filename;
    if(argc >= 3)
        final_filename = argv[2];
    else {
        final_filename = url.substr(url.find_last_of("/") + 1);
        if(final_filename.empty())
            final_filename = "downloaded_file";
    }
    
    // 設定預設值
    int num_segments = 4;  // 預設使用 4 個分段
    
    // For HTTPS use port 443.
    std::string port = "443";

    // Set up an SSL context.
    ssl::context ctx(ssl::context::sslv23_client);
    ctx.set_default_verify_paths();
    ctx.set_verify_mode(ssl::verify_none); // 停用 SSL 驗證

    // 解析 URL，取得 host 與 target
    auto parsed_url = parse_url(url);
    
    // Get the total size via a HEAD request.
    std::size_t total_size = get_content_length_https(parsed_url.host, port, parsed_url.target, ctx);
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
        threads.emplace_back(download_segment_https, parsed_url.host, port, parsed_url.target, 
                           start, end, seg_file, std::ref(ctx));
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
