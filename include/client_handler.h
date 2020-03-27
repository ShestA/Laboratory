// Copyright 2020 <DreamTeamGo>

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
using namespace boost::asio;
using namespace boost::posix_time;

struct SyncServer : boost::enable_shared_from_this<SyncServer> {
public:
    SyncServer();

public:
    void answer_to_client();
    bool timed_out() const;

    void set_clients_changed() { on_changed = true; }
    ip::tcp::socket & sock() { return tcp_sock; }

private:
    std::string username() const { return login; }
    void stop();
    void read_request();
    void process_request();
    
    void process_message(const std::string & msg);
    void on_login();
    void on_ping();
    void on_clients();


    void write(const std::string & msg);

private:
    ip::tcp::socket tcp_sock;
    enum { max_msg = 1024 };
    int read_count;
    char buff[max_msg];
    std::string login;
    bool on_changed;
    ptime last_ping;
};

class SyncServerImpl
{
public:
    static void run_server();

private:
    void accept_thread();
    void handle_clients_thread();
};
