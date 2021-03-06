#pragma once

#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <driver.h>

namespace roflan_srv {

class TcpConnection : public boost::enable_shared_from_this<TcpConnection> {
public:
    typedef boost::shared_ptr<TcpConnection> pointer;

    explicit TcpConnection(boost::asio::io_service& io_service);

    static pointer create(boost::asio::io_service& io_service) {
        return pointer(new TcpConnection(io_service));
    }

    boost::asio::ip::tcp::socket& socket();

    void start();
    void handle_write(const boost::system::error_code& , size_t /*bytes_transferred*/);
    void handle_read(const boost::system::error_code& , size_t /*bytes_transferred*/);

private:
    roflan_parser::Driver parser_driver_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::streambuf query_;
};

}
// //namespace roflan_srv
