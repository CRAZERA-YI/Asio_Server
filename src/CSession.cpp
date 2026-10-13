#include "../include/CSession.h"
#include "../include/CServer.h"
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/date_time/gregorian/greg_day.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/system/error_code.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <sys/socket.h>

CSession::CSession(boost::asio::io_context &ioc, CServer *server)
    : _server(server), _socket(ioc) {
    boost::uuids::uuid temp_uuid = boost::uuids::random_generator()();
    _uuid = boost::lexical_cast<std::string>(temp_uuid);
    _recv_head_node = make_shared<MsgNode>(HEAD_LENGTH);
    // _recv_msg_node = make_shared<MsgNode>();
}

tcp::socket &CSession::GetSocket() {
    return _socket;
}

std::string &CSession::GetUuid() {
    return _uuid;
}

void CSession::Start() {
    memset(_data, 0, MAX_LENGTH);
    _socket.async_read_some(
        boost::asio::buffer(_data, MAX_LENGTH),
        std::bind(&CSession::HandleRead, this, std::placeholders::_1,
                  std::placeholders::_2, shared_from_this()));
}

void CSession::Send(char *msg, int max_length) {
    bool pending = false;
    std::lock_guard<std::mutex> lock(_send_lock);
    if (_send_que.size() > 0) {
        pending = true;
    }
    _send_que.push(make_shared<MsgNode>(msg, max_length));
    if (pending) {
        return;
    }

    boost::asio::async_write(_socket, boost::asio::buffer(msg, max_length),
                             std::bind(&CSession::HandleWrite, this,
                                       std::placeholders::_1,
                                       shared_from_this()));
}

void CSession::HandleWrite(const boost::system::error_code &error,
                           shared_ptr<CSession> _self_shared) {
    if (!error) {
        std::lock_guard<std::mutex> lock(_send_lock);
        _send_que.pop();
        if (!_send_que.empty()) {
            auto &msgnode = _send_que.front();
            boost::asio::async_write(
                _socket, boost::asio::buffer(msgnode->_data, msgnode->_max_len),
                std::bind(&CSession::HandleWrite, this, std::placeholders::_1,
                          _self_shared));
        }
    } else {
        std::cout << "出了点问题 : " << error.what() << endl;
        _server->ClearSession(_uuid);
    }
}
/*
void CSession::HandleRead(const boost::system::error_code &error,
                          size_t bytes_transferred,
                          shared_ptr<CSession> _self_shared) {
    if (!error) {

        cout << "收到的数据为 : " << "[" << _data << "]" << endl;
        //
        Send(_data, bytes_transferred);
        memset(_data, 0, MAX_LENGTH);
        _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                std::bind(&CSession::HandleRead, this,
                                          std::placeholders::_1,
                                          std::placeholders::_2, _self_shared));
    } else {
        std::cout << "出了点问题 : " << error.what() << endl;

        _server->ClearSession(_uuid);
    }
}
*/

void CSession::HandleRead(const boost::system::error_code &error,
                          size_t bytes_transferred,
                          shared_ptr<CSession> _self_shared) {
    if (!error) {
        int copy_len = 0;
        while (bytes_transferred > 0) {
            if (!_b_head_parse) {
                if (bytes_transferred + _recv_head_node->_cur_len <
                    HEAD_LENGTH) {
                    memcpy(_recv_head_node->_data + _recv_head_node->_cur_len,
                           _data + copy_len, bytes_transferred);
                    _recv_head_node->_cur_len += bytes_transferred;
                    memset(_data, 0, MAX_LENGTH);
                    _socket.async_read_some(
                        boost::asio::buffer(_data, MAX_LENGTH),
                        std::bind(&CSession::HandleRead, this,
                                  std::placeholders::_1, std::placeholders::_2,
                                  shared_from_this()));
                }

                int head_remain = HEAD_LENGTH - _recv_head_node->_cur_len;
                memcpy(_recv_head_node->_data + _recv_head_node->_cur_len,
                       _data, head_remain);
                copy_len += head_remain;
                bytes_transferred -= head_remain;
                short data_len = 0;
                memcpy(&data_len, _recv_head_node->_data, HEAD_LENGTH);

                if (data_len > MAX_LENGTH) {
                    std::cout << "接受的数据过长,不予接受,断开服务"
                              << std::endl;
                    _server->ClearSession(_uuid);
                    return;
                }
                _recv_msg_node = make_shared<MsgNode>(data_len);

                if (bytes_transferred < data_len) {
                    memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len,
                           _data + copy_len, bytes_transferred);
                    _recv_msg_node->_cur_len += bytes_transferred;
                    memset(_data, 0, MAX_LENGTH);
                    _socket.async_read_some(
                        boost::asio::buffer(_data, MAX_LENGTH),
                        std::bind(&CSession::HandleRead, this,
                                  std::placeholders::_1, std::placeholders::_2,
                                  shared_from_this()));
                    _b_head_parse = true;
                    return;
                }

                memcpy(_recv_msg_node->_data + _recv_head_node->_cur_len,
                       _data + copy_len, data_len - _recv_msg_node->_cur_len);
                _recv_msg_node->_cur_len += data_len;
                bytes_transferred -= data_len;
                _recv_msg_node->_data[_recv_msg_node->_max_len] = '\0';
                std::cout << "接受到的数据内容为 : " << _recv_msg_node->_data
                          << std::endl;
                Send(_recv_msg_node->_data, _recv_msg_node->_max_len);
                // memset(_data,0,MAX_LENGTH);
                //_recv_msg_node->Clear();
                _b_head_parse = false;
                _recv_head_node->Clear();
                if (bytes_transferred <= 0) {
                    memset(_data, 0, MAX_LENGTH);
                    _socket.async_read_some(
                        boost::asio::buffer(_recv_head_node->_data,
                                            HEAD_LENGTH),
                        std::bind(&CSession::HandleRead, this,
                                  std::placeholders::_1, std::placeholders::_2,
                                  shared_from_this()));
                    return;
                }

                continue;
            }

            int msg_remain =
                _recv_msg_node->_max_len - _recv_msg_node->_cur_len;
            if (bytes_transferred < msg_remain) {
                memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len,
                       _data + copy_len, bytes_transferred);
                _recv_msg_node->_cur_len += bytes_transferred;
                ::memset(_data, 0, MAX_LENGTH);
                _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                        std::bind(&CSession::HandleRead, this,
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  shared_from_this()));
                return;
            }

            memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data,
                   msg_remain);
            bytes_transferred -= msg_remain;
            _recv_msg_node->_cur_len += msg_remain;
            _recv_msg_node->_data[_recv_msg_node->_max_len] = '\0';
            std::cout << "接受到的数据内容为 : " << _recv_msg_node->_data
                      << std::endl;
            Send(_recv_msg_node->_data, _recv_msg_node->_max_len);
            _b_head_parse = false;
            _recv_head_node->Clear();

            if (bytes_transferred <= 0) {
                ::memset(_data, 0, MAX_LENGTH);
                _socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
                                        std::bind(&CSession::HandleRead, this,
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  shared_from_this()));
                return;
            }
            continue;
        }
    } else {
        std::cout << "出了点问题 : " << error.message() << std::endl;
        Close();
        _server->ClearSession(_uuid);
    }
}
