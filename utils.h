#pragma once

#include "stdafx.h"
#define DEBUG 1

using namespace std;

bool isSameHost(string hostname1, char* hostname2) {
  const char* core_name = hostname1.substr(0, hostname1.find(":")).c_str();
  if (strcmp(core_name, hostname2) == 0)
    return true;
  else
    return false;
}

class NodeMgmt {
public: 
  NodeMgmt(string _hostname, string mServerInfo) {
    this->hostname = _hostname;
    this->masterStub = Master::NewStub(grpc::CreateChannel(
      mServerInfo, grpc::InsecureChannelCredentials()));
    this->joinMaster();
  }

  NodeMgmt(string _hostname) {
    this->hostname = _hostname;
  }

  string getHostName() {
    return this->hostname;
  }

  void broadcastLogin(string uname) {
    for (auto itr = msgServerStubs.begin(); itr != msgServerStubs.end(); itr++) {
      unique_ptr<MessengerServer::Stub>& msgStub = *itr;
      ClientContext context;
      NodeReq nodeReq;
      Reply reply;

      nodeReq.add_msg(uname);
      nodeReq.set_src(this->hostname);

      Status status = msgStub->NotifyLogin(&context, nodeReq, &reply);
      if (status.ok())
        continue;
      else
        cout << "error in notifying login" << endl;
    }
  }

  void sync(SyncMsg msg) {
    for (auto itr = msgServerStubs.begin(); itr != msgServerStubs.end(); itr++) {
      unique_ptr<MessengerServer::Stub>& msgStub = *itr;
      ClientContext context;
      Reply reply;

      Status status = msgStub->Sync(&context, msg, &reply);
      if (status.ok())
        continue;
      else
        cout << "error in sync " << msg.cmd() << endl;
    }
  }

  void joinMaster() {
    ClientContext context;
    Request request;
    Reply reply;
    request.add_arguments(this->hostname);

    Status status = this->masterStub->RegisterSlave(&context, request, &reply);
    if (status.ok()) {
#ifdef DEBUG
      cout << this->hostname << " joinMaster succeeded!" << endl;
#endif // DEBUG
      for (int i = 0; i < reply.arguments_size(); i++) {
#ifdef DEBUG
        cout << "Connecting to " << reply.arguments(i) << endl;
#endif // DEBUG
        this->msgServerStubs.push_back(MessengerServer::NewStub(grpc::CreateChannel(
          reply.arguments(i), grpc::InsecureChannelCredentials())));
      }
      return;
    }
    else {
#ifdef DEBUG
      cout << this->hostname << " joinMaster failed!" << endl;
#endif // DEBUG
      return;
    }
  }

  void registerSlave(string msgServerInfo) {
    this->msgServerStubs.push_back(MessengerServer::NewStub(grpc::CreateChannel(
      msgServerInfo, grpc::InsecureChannelCredentials())));
  }

protected:
  vector<unique_ptr<MessengerServer::Stub>> msgServerStubs;
  unique_ptr<Master::Stub> masterStub;
  // this contains "hostname:port"
  string hostname;
};

class MasterMgmt : NodeMgmt{
public:
  MasterMgmt(string _hostname) : NodeMgmt(_hostname){}

  string getRRTerm() {
    string server = servers[this->rr_term];
    this->rr_term = (++this->rr_term) % servers.size();
    return server;
  }

  void registerMsgServer(string msgServerInfo) {
    // TODO 
    // 1. add it into server stubs collection
    // 2. broadcast newly registered server to all already-registered servers

#ifdef DEBUG
    cout << "In function mastermgmt->registerMsgServer before broadcasting!" << endl;
#endif // DEBUG
    Request request;
    request.add_arguments(msgServerInfo);
    Status status;

    for (vector<unique_ptr<MessengerServer::Stub>>::iterator itr = msgServerStubs.begin();
      itr != msgServerStubs.end(); itr++) {

      ClientContext context;
      
      Reply br_reply;

      unique_ptr<MessengerServer::Stub>& stub = *itr;
      status = stub->RegisterSlave(&context, request, &br_reply);
      if (status.ok())
      {
        cout << "register server " << msgServerInfo
          << " on " << br_reply.msg() << " succeeded!" << endl;
      }
    }
#ifdef DEBUG
    cout << "Pushing stub to " << msgServerInfo << " into list!" << endl;
#endif // DEBUG

    msgServerStubs.push_back(MessengerServer::NewStub(grpc::CreateChannel(
      msgServerInfo, grpc::InsecureChannelCredentials())));
    servers.push_back(msgServerInfo);
  }

  vector<string> getServerList() {
    return this->servers;
  }

private: 
  int rr_term = 0;
  vector<string> servers;
};