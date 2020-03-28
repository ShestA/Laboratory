// Copyright 2020 <DreamTeamGo>
#include <client_base.h>
#include <logger.h>

#include <iostream>

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace
{
    io_service service;
    const int MAX_ATTEMPTS = 5;
    const int MIN_ATTEMPTS = 0;
}

ClientBase::ClientBase(const User& user)
        : tcp_sock(service), is_started(true), user(user) {}
void ClientBase::connect(ip::tcp::endpoint ep)
{
    tcp_sock.connect(ep);
}

void ClientBase::loop() {
    write("login " + user.login + "\n");
    read_answer();
    while (is_started) {
        write_request();
        read_answer();
        int millis = rand() % 7000;
        BOOST_LOG_TRIVIAL(info) << user.login << " postpone ping "
                  << millis << " ms";
        boost::this_thread::sleep(boost::posix_time::millisec(millis));
    }
}

void ClientBase::write_request() {
    write("ping\n");
}

void ClientBase::read_answer() {
    read_count = 0;
    read(tcp_sock, buffer(buff), boost::bind(&ClientBase::read_complete, this, _1, _2));
    process_msg();
}

void ClientBase::process_msg() {
    std::string msg(buff, read_count);
    if ( msg.find("login ") == 0) on_login();
    else if ( msg.find("ping") == 0) on_ping(msg);
    else if ( msg.find("clients ") == 0) on_clients(msg);
    else std::cerr << "invalid msg " << msg;
}

void ClientBase::on_login() {
    BOOST_LOG_TRIVIAL(info) << user.login << " logged in";
    user.attempts = 0;
    do_ask_clients();
}

void ClientBase::on_ping(const std::string & msg) {
    std::istringstream in(msg);
    std::string answer;
    in >> answer >> answer;
    if (answer == "client_list_changed")
        do_ask_clients();
}

void ClientBase::on_clients(const std::string & msg) {
    std::string clients = msg.substr(8);
    BOOST_LOG_TRIVIAL(info) << user.login << ", new client list:" << clients;
}

void ClientBase::do_ask_clients() {
    write("ask_clients\n");
    read_answer();
}

void ClientBase::write(const std::string & msg) {
    tcp_sock.write_some(buffer(msg));
}

size_t ClientBase::read_complete(const boost::system::error_code & err, size_t bytes) {
    if ( err) return 0;
    read_count = bytes;
    bool found = std::find(buff, buff + bytes, '\n') < buff + bytes;
    return found ? 0 : 1;
}

void Client::run_client(const std::string & client_name, int attempts) {
    if (attempts >= MAX_ATTEMPTS)
    {
        BOOST_LOG_TRIVIAL(info) << "Unable to reconnect " << client_name.c_str();
        return;
    }
    User usr { client_name, attempts };
    ClientBase client(usr);
    try {
        client.connect({ip::address::from_string("127.0.0.1"), 8001});
        client.loop();
    } catch(boost::system::system_error & err) {
        BOOST_LOG_TRIVIAL(info) << "client terminated " << client.get_user().login << ": " << err.what();
        BOOST_LOG_TRIVIAL(info) << "Trying to reconnect " << client.get_user().login << " attempts: " << client.get_user().attempts + 1;
        run_client(client.get_user().login, client.get_user().attempts + 1);
    }
}


void Client::create_clients(const char *clients[], const int num)
{
    Logger::init("./client_log");
    BOOST_LOG_TRIVIAL(info) <<  "create_clients";
    boost::thread_group threads;
    Client cl;
    for ( int i = 0; i < num; i++ ) {
        threads.create_thread(boost::bind(&Client::run_client, &cl, clients[i], MIN_ATTEMPTS));
        boost::this_thread::sleep(boost::posix_time::millisec(100));
    }
    threads.join_all();
}
