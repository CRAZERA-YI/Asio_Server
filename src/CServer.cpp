#include "../include/CServer.h"
#include "../include/CSession.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/detail/error_code.hpp>
#include <functional>
#include <iostream>
#include <memory>

#include <utility>

CServer::CServer(boost::asio::io_context &io_context, short port)
    : _io_context(io_context),
      _acceptor(io_context, boost::asio::ip::tcp::endpoint(
                                boost::asio::ip::tcp::v4(), port)) {
    StartAccept();
}

void CServer::HandleAccept(shared_ptr<CSession> new_session,
                           const boost::system::error_code &error) {

    if (!error) {
        std::string client_ip =
            new_session->GetSocket().remote_endpoint().address().to_string();
        cout << "新设备连入,IP : " << client_ip << std::endl;
        new_session->Start();
        _sessions.insert(std::make_pair(new_session->GetUuid(), new_session));

    } else {
        cout << "出了点问题 : " << error.message() << std::endl;
    }

    StartAccept();
}

void CServer::StartAccept() {
    auto new_session = make_shared<CSession>(_io_context, this);
    _acceptor.async_accept(
        new_session->GetSocket(),
        bind(&CServer::HandleAccept, this, new_session, std::placeholders::_1));
}

void CServer::ClearSession(std::string uuid) {
    _sessions.erase(uuid);
}
