#ifndef __CSESSION_H
#define __CSESSION_H
#include <boost/asio.hpp>
#include <cstring>
#include <iostream>
#include <mutex>
#include <queue>

using namespace std;
using namespace boost::asio::ip;
const int MAX_LENGTH = 1024;
const int HEAD_LENGTH = 2;

class CServer;

class MsgNode {
    friend class CSession;

  public:
    MsgNode(char *msg, int max_len)
        : _cur_len(0), _max_len(max_len + HEAD_LENGTH) {
        _data = new char[_max_len + 1];
        memcpy(_data, &max_len, HEAD_LENGTH);
        memcpy(_data + HEAD_LENGTH, msg, max_len);
        _data[_max_len] = '\0';
    }

    MsgNode(int max_len) : _max_len(max_len), _cur_len(0) {
        _data = new char[_max_len + 1]();
    }

    ~MsgNode() {
        delete[] _data;
    }

    void Clear() {
        memset(_data, 0, _max_len);
        _cur_len = 0;
    }

  private:
    int _cur_len;
    int _max_len;
    char *_data;
};
class CSession : public std::enable_shared_from_this<CSession> {
  public:
    CSession(boost::asio::io_context &io_context, CServer *server);
    ~CSession() {
        std::cout << "会话已被销毁 " << endl;
    }
    tcp::socket &GetSocket();
    std::string &GetUuid();
    void Start();
    void Send(char *msg, int max_length);
    void Close() {
        _socket.close();
        _b_close = true;
    }

  private:
    void HandleRead(const boost::system::error_code &error,
                    size_t bytes_transferred,
                    shared_ptr<CSession> _self_shared);
    void HandleWrite(const boost::system::error_code &error,
                     shared_ptr<CSession> _self_shared);
    tcp::socket _socket;
    std::string _uuid;
    char _data[MAX_LENGTH];
    CServer *_server;
    bool _b_close;
    std::queue<shared_ptr<MsgNode>> _send_que;
    std::mutex _send_lock;
    // 收到的消息结构
    std::shared_ptr<MsgNode> _recv_msg_node;
    bool _b_head_parse;
    // 收到的头部结构
    std::shared_ptr<MsgNode> _recv_head_node;
};

#endif
