// Copyright 2020 <DreamTeamGo>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
using namespace boost::asio;

struct User
{
    std::string login;
    int attempts;
};

struct ClientBase {
public:
    ClientBase(const User& user);
    void connect(ip::tcp::endpoint ep);
    void loop();
    User get_user() const { return user; }
private:
    void write_request();
    void read_answer();
    void process_msg();

    void on_login();
    void on_ping(const std::string & msg);
    void on_clients(const std::string & msg);
    void do_ask_clients();

    void write(const std::string & msg);
    size_t read_complete(const boost::system::error_code & err, size_t bytes);

private:
    ip::tcp::socket tcp_sock;
    enum { max_msg = 1024 };
    int read_count;
    char buff[max_msg];
    bool is_started;
    User user;
};

class Client
{
public:
    static void create_clients(const char *clients[], const int num);
private:
    void run_client(const std::string & client_name, int attempts);
};
