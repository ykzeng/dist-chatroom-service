#pragma once
#ifndef UTILS_H
#define UTILS_H

#include "stdafx.h"
#define DEBUG 1

using namespace std;

// abstract definition
class MessengerServiceImpl final : public MessengerServer::Service {
  Status RegisterSlave(ServerContext* context, const JoinRequest* request, Reply* reply) override;
  Status Heartbeat(ServerContext* context, const Foo* request, Foo* reply) override;
  Status Sync(ServerContext* context, const SyncMsg* msg, Reply* reply) override;
  Status List(ServerContext* context, const Request* request, ListReply* list_reply) override;
  Status Join(ServerContext* context, const Request* request, Reply* reply) override;
  Status Leave(ServerContext* context, const Request* request, Reply* reply) override;
  Status Login(ServerContext* context, const Request* request, Reply* reply) override;
  Status Chat(ServerContext* context,
    ServerReaderWriter<Message, Message>* stream) override;

};

void print_strvec(vector<string> toprint) {
  for (size_t i = 0; i < toprint.size(); i++)
  {
    cout << "The " << i << "th element: " << toprint[i] << endl;
  }
}

//Client struct that holds a user's username, followers, and users they follow
struct Client {
  std::string username;
  bool connected = true;
  int following_file_size = 0;
  std::vector<Client*> client_followers;
  std::vector<Client*> client_following;
  ServerReaderWriter<Message, Message>* stream = 0;
  //string host;
  // flag that indicates if the client is on this server
  bool onServer = false;
  bool operator==(const Client& c1) const {
    return (username == c1.username);
  }
};

class CMD {
public:
  static const string JOIN;
  static const string LOGIN;
  static const string LEAVE;
  static const string CHAT;
  static const string DISCONN;
};

const string CMD::JOIN = "JOIN";
const string CMD::LOGIN = "LOGIN";
const string CMD::LEAVE = "LEAVE";
const string CMD::CHAT = "CHAT";
const string CMD::DISCONN = "DISCONN";

bool isSameHost(string hostname1, char* hostname2) {
  const char* core_name = hostname1.substr(0, hostname1.find(":")).c_str();
  if (strcmp(core_name, hostname2) == 0)
    return true;
  else
    return false;
}

string genRecord(Message message) {
  // get the info for writing to file
  google::protobuf::Timestamp temptime = message.timestamp();
  std::string time = google::protobuf::util::TimeUtil::ToString(temptime);
  std::string fileinput = time + " :: " + message.username() + ":" + message.msg() + "\n";
  return fileinput;
}

void distributeChats(Client* c, Message message, bool isReplica) {
  string username = c->username;
  // get the info for writing to file
  string fileinput = genRecord(message);

  if (!isReplica)
  {
    //Write the current message to "username.txt"
    std::string filename = username + ".txt";
    std::ofstream user_file(filename, std::ios::app | std::ios::out | std::ios::in);
    user_file << fileinput;

#ifdef DEBUG
    cout << username + ".txt is now written" << endl;
#endif // DEBUG
  }

  //persist the message to each follower's file
  //send msg to each local follower's stream
  std::vector<Client*>::const_iterator it;
  for (it = c->client_followers.begin(); it != c->client_followers.end(); it++) {
    Client *temp_client = *it;
    // for those local followers, write to stream
    if (temp_client->onServer) {
#ifdef DEBUG
      cout << "=====writing " << message.msg() << " to " << temp_client->username << endl;
#endif // DEBUG

      assert(temp_client->stream != 0 && temp_client->connected);
      temp_client->stream->Write(message);
    }

    if (!isReplica) {
      //For each of the current user's followers, put the message in their following.txt file
      std::string temp_file = temp_client->username + "following.txt";
      std::ofstream following_file(temp_file, std::ios::app | std::ios::out | std::ios::in);
      following_file << fileinput;
#ifdef DEBUG
      cout << "=======wrote to " << temp_file << endl;
#endif // DEBUG
      // following file record count ++
      temp_client->following_file_size++;

      std::ofstream user_file(temp_username + ".txt", std::ios::app | std::ios::out | std::ios::in);
      user_file << fileinput;
#ifdef DEBUG
      cout << "=======wrote to " << temp_username << endl;
#endif // DEBUG
    }
  }
}

class NodeMgmt {
public: 
  NodeMgmt(string _hostname, string mServerInfo, bool _isReplica) {
    this->hostname = _hostname;
    this->masterStub = Master::NewStub(grpc::CreateChannel(
      mServerInfo, grpc::InsecureChannelCredentials()));
    this->replica = _isReplica;
    this->joinMaster();
  }

  NodeMgmt(string _hostname) {
    this->hostname = _hostname;
  }

  string getHostName() {
    return this->hostname;
  }

  void sync(SyncMsg msg) {
    for (auto itr = msgServerStubs.begin(); itr != msgServerStubs.end(); itr++) {
      unique_ptr<MessengerServer::Stub>& msgStub = *itr;
      ClientContext context;
      Reply reply;
#ifdef DEBUG
      cout << "in function nodemgmt->sync" << endl;
#endif // DEBUG

      Status status = msgStub->Sync(&context, msg, &reply);
      if (status.ok())
        cout << "success in sync " << msg.cmd() << " on " << reply.msg() << endl;
      else
        cout << "error in sync " << msg.cmd() << " on " << reply.msg() << endl;
    }
  }

  //string writeUserChats(Message message) {
  //  string fileinput, username = message.username();
  //  string filename = username + ".txt";
  //  ofstream user_file(filename, std::ios::app | std::ios::out | std::ios::in);
  //  google::protobuf::Timestamp temptime = message.timestamp();
  //  string time = google::protobuf::util::TimeUtil::ToString(temptime);
  //  fileinput = time + " :: " + message.username() + ":" + message.msg() + "\n";
  //  user_file.close();
  //  return fileinput;
  //}

  //void distributeChats(vector<Client*> followers, Message msg, string fileinput) {
  //  for (auto itr = followers.begin(); itr != followers.end(); itr++) {
  //    Client* tClient = *itr;
  //    // 1. write to the files anyway
  //    string tUname = tClient->username;
  //    string tFileName = tUname + "following.txt";
  //    ofstream foOut(tFileName, std::ios::app | std::ios::out | std::ios::in);
  //    foOut << fileinput;
  //    tClient->following_file_size++;
  //    foOut.close();
  //    ofstream userOut(tUname + ".txt", std::ios::app | std::ios::out | std::ios::in);
  //    userOut << fileinput;
  //    userOut.close();
  //    // 2. distribute to client stream if the client is on this server
  //    if (tClient->onServer) {
  //      // then the client must be connected, i.e., stream neq 0, connected yes
  //      assert(tClient->stream != 0 && tClient->connected);
  //      tClient->stream->Write(msg);
  //    }
  //  }
  //}

  void joinMaster() {
    ClientContext context;
    JoinRequest request;
    JoinReply reply;
    request.set_hostname(this->getHostName());
    request.set_replica(this->isReplica);

    Status status = this->masterStub->RegisterSlave(&context, request, &reply);
    if (status.ok()) {
//#ifdef DEBUG
//      cout << this->hostname << " joinMaster succeeded as a "
//        << (isReplica ? "replica server":"server") << endl;
//#endif // DEBUG
      for (int i = 0; i < reply.slaves_size(); i++) {
#ifdef DEBUG
        cout << "Newly Joined Server Connecting to " << reply.slaves(i) << endl;
#endif // DEBUG
        string tmpSlave = reply.slaves(i);
        this->msgServerStubs.push_back(MessengerServer::NewStub(grpc::CreateChannel(
          tmpSlave, grpc::InsecureChannelCredentials())));
        this->allSlaves.push_back(tmpSlave);
      }
      /*for (int i = 0; i < reply.workers_size(); i++) {
        string tmpWorker = reply.workers(i);
        this->workers.push_back(tmpWorker);
      }*/
      return;
    }
    else {
#ifdef DEBUG
      cout << this->hostname << " joinMaster failed!" << endl;
#endif // DEBUG
      return;
    }
  }

  void runChatServer() {
#ifdef DEBUG
    cout << "run chat server on " << this->hostname << endl;
#endif // DEBUG

    std::string server_address = this->hostname;
    MessengerServiceImpl service;

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    this->server = builder.BuildAndStart();
    std::cout << "Chat Server listening on " << server_address << std::endl;
    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
  }

  void registerSlave(string msgServerInfo) {
    this->msgServerStubs.push_back(MessengerServer::NewStub(grpc::CreateChannel(
      msgServerInfo, grpc::InsecureChannelCredentials())));
    allSlaves.push_back(msgServerInfo);
  }

  vector<string> getAllSlaves() {
    return this->allSlaves;
  }

  bool isReplica() {
    return this->replica;
  }

  void heartbeat() {
    // Sequential identification
    // TODO very bad in large concurrent system
    int pos = find(allSlaves.begin(), allSlaves.end(), this->hostname) - allSlaves.begin();
#ifdef DEBUG
    cout << "the chat master position in stub vector: " << pos << endl;
#endif // DEBUG
    unique_ptr<MessengerServer::Stub>& chatMaster = msgServerStubs[pos];
    Foo req;
    while (true)
    {
      ClientContext context;
      Foo reply;
      // heartbeat every 25s
      usleep(25 * 1000);
      Status status = chatMaster->Heartbeat(&context, req, &reply);
      if (!status.ok()) {
        // recover the chat master
        // 1. fork myself
        // 2. modify isReplica
        // 3. get registered client list, set their onserver property to be true
        // 4. modify the port
        
      }
#ifdef DEBUG
      else {
        cout << "Heartbeat succeeded!" << endl;
      }
#endif // DEBUG

    }
  }

protected:
  vector<string> allSlaves;
  vector<unique_ptr<MessengerServer::Stub>> msgServerStubs;
  unique_ptr<Master::Stub> masterStub;
  unique_ptr<Server> server;
  // this contains "hostname:port"
  string hostname;
  bool replica;
};

class MasterMgmt : public NodeMgmt{
public:
  MasterMgmt(string _hostname) : NodeMgmt(_hostname){}

  /*string getRRTerm() {
    string server = workers[this->rr_term];
    this->rr_term = (++this->rr_term) % workers.size();
    return server;
  }*/

  vector<string> getWorkerList() {
    return this->workers;
  }

  string rrServerAssign(string uname) {
    // TODO how to ensure that the server assignment is success?
    // one thing we can probably do is to check connection before assignment
    // if conn failed, then remove server from the list temporarily, assign 
    // another server to the client
    int index = this->rr_term;
    string server = workers[index];
    this->rr_term = (++this->rr_term) % workers.size();
    csMap.insert(pair<string, int>(uname, index));
    return server;
  }

  void registerMsgServer(string msgServerInfo, bool isReplica) {
    // TODO 
    // 1. add it into server stubs collection
    // 2. broadcast newly registered server to all already-registered servers

#ifdef DEBUG
    cout << "In function mastermgmt->registerMsgServer before broadcasting!" << endl;
#endif // DEBUG
    JoinRequest request;
    request.set_hostname(msgServerInfo);
    request.set_replica(isReplica);

    for (auto itr = msgServerStubs.begin();
      itr != msgServerStubs.end(); itr++) {

      ClientContext context;
      
      Reply br_reply;

      unique_ptr<MessengerServer::Stub>& stub = *itr;

      Status status = stub->RegisterSlave(&context, request, &br_reply);
      if (status.ok())
      {
        cout << "register server " << msgServerInfo
          << " on " << br_reply.msg() << " succeeded!" << endl;
      }
    }

    msgServerStubs.push_back(MessengerServer::NewStub(grpc::CreateChannel(
      msgServerInfo, grpc::InsecureChannelCredentials())));
#ifdef DEBUG
    cout << "Pushed stub to " << msgServerInfo << " into list!" << endl;
#endif // DEBUG
    // push worker/replica addr to vector
    if (!isReplica) {
#ifdef DEBUG
      cout << msgServerInfo << " is not a replica" << endl;
#endif // DEBUG
      workers.push_back(msgServerInfo);
    }
    else {
#ifdef DEBUG
      cout << msgServerInfo << " is a replica" << endl;
#endif // DEBUG
    }
    allSlaves.push_back(msgServerInfo);
  }

  //void heartbeat() {
  //  while (true)
  //  {
  //    for (auto itr = msgServerStubs.begin(); itr != msgServerStubs.end(); itr++) {
  //      unique_ptr<MessengerServer::Stub>& stub = *itr;
  //      Status status = stub->Heartbeat();
  //      // if not receiving proper reply
  //      if (!status.ok())
  //      {
  //        
  //      }
  //    }
  //    // sleep 20 seconds
  //    usleep(20*1000);
  //  }
  //}

private:
  vector<string> workers;
  int rr_term = 0;
  // client to server stub index map
  map<string, int> csMap;
};


#endif // !1