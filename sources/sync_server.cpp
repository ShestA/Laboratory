// Copyright 2020 <DreamTeamGo>

#include <client_handler.h>
#include <logger.h>

#include <iostream>

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

using  boost::asio;
using  boost::posix_time;

using client_ptr = std::shared_ptr<SyncServer>;
using array = std::vector<client_ptr>;

namespace
{
    io_service service;
    array clients;
    boost::recursive_mutex mutex;
}

namespace server_detail
{
    void update_clients_changed()
    {
        boost::recursive_mutex::scoped_lock lk(mutex);
        for (auto& client : clients)
            client->set_clients_changed();
    }
}

SyncServer::SyncServer() : tcp_sock(service), read_count(0)
{
    last_ping = microsec_clock::local_time();
}

void SyncServer::answer_to_client()
{
    try
    {
        read_request();
        process_request();
    }
    catch (boost::system::system_error&)
    {
        stop();
    }
    if (timed_out())
    {
        stop();
        BOOST_LOG_TRIVIAL(info) << "stopping " << login << " - no ping in time";
    }
}

bool SyncServer::timed_out() const
{
    ptime now = microsec_clock::local_time();
    __uint64_t ms = (now - last_ping).total_milliseconds();
    return ms > 5000;
}

void SyncServer::stop()
{
    boost::system::error_code err;
    tcp_sock.close(err);
}

void SyncServer::read_request()
{
    if (tcp_sock.available())
        read_count += tcp_sock.read_some(buffer(buff + read_count, max_msg - read_count));
}

void SyncServer::process_request()
{
    bool found_enter = std::find(buff, buff + read_count, '\n') < buff + read_count;
    if (!found_enter)
        return;

    last_ping = microsec_clock::local_time();
    size_t pos = std::find(buff, buff + read_count, '\n') - buff;
    std::string msg(buff, pos);
    std::copy(buff + read_count, buff + max_msg, buff);
    read_count -= pos + 1;

    process_message(msg);
}

void SyncServer::process_message(const std::string & msg)
{
    std::istringstream input(msg);

    std::string type;
    input >> type;
    if ("login" == type)
    {
        input >> login;
        on_login();
    }
    else if ("ping" == type) on_ping();
    else if ("ask_clients" == type) on_clients();
    else
    {
        BOOST_LOG_TRIVIAL(error) << "invalid msg " << msg;
    }
}

void SyncServer::on_login()
{
    BOOST_LOG_TRIVIAL(info) << login << " logged in";
    write("login ok\n");
    server_detail::update_clients_changed();
}

void SyncServer::on_ping()
{
    write(on_changed ? "ping client_list_changed\n" : "ping ok\n");
    on_changed = false;
}

void SyncServer::on_clients()
{
    std::string msg;
    {
        boost::recursive_mutex::scoped_lock lk(mutex);
        for (auto& client : clients)
            msg += client->username() + " ";
    }
    write("clients " + msg + "\n");
}

void SyncServer::write(const std::string & msg)
{
    tcp_sock.write_some(buffer(msg));
}

void SyncServerImpl::accept_thread()
{
    ip::tcp::acceptor acceptor {service, ip::tcp::endpoint {ip::tcp::v4(), 8001}};
    while (true)
    {
        auto client = std::make_shared<SyncServer>();
        acceptor.accept(client->sock());
        boost::recursive_mutex::scoped_lock lock{mutex};
        clients.push_back(client);
    }
}

void SyncServerImpl::handle_clients_thread()
{
    while (true)
    {
        boost::this_thread::sleep(millisec(1));
        boost::recursive_mutex::scoped_lock lk(mutex);
        for (auto& client : clients)
            client->answer_to_client();
        clients.erase(std::remove_if(clients.begin(), clients.end(),
            boost::bind(&SyncServer::timed_out, _1)), clients.end());
    }
}

void SyncServerImpl::run_server()
{
    Logger::init("./sync_server");
    boost::thread_group threads;
    SyncServerImpl serv;
    threads.create_thread(boost::bind(&SyncServerImpl::accept_thread, &serv));
    threads.create_thread(boost::bind(&SyncServerImpl::handle_clients_thread, &serv));
    threads.join_all();
}
