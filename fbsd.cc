/*
 *
 * Copyright 2015, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*Will Adams and Nicholas Jackson
  CSCE 438 Section 500*/

#include "stdafx.h"
#define DEBUG 1

//Vector that stores every client that has been created
vector<Client> client_db;
MasterMgmt* masterMgmt = nullptr;
NodeMgmt* nodeMgmt = nullptr;

class MasterServer final : public Master::Service {
  Status RequestServer(ServerContext* context, const Request* request, Reply* reply) {
    // TODO rewrite the logic using object here
    string server = masterMgmt->rrServerAssign(request->username());
    reply->set_msg(server);
    return Status::OK;
  }

  Status RegisterSlave(ServerContext* context, const Request* request, Reply* reply) {
    masterMgmt->registerMsgServer(request->arguments(0));
    vector<string> serverList = masterMgmt->getServerList();
    for (auto itr = serverList.begin(); itr != (serverList.end() - 1); itr++) {
      reply->add_arguments(*itr);
    }

    return Status::OK;
  }
};

//Helper function used to find a Client object given its username
int find_user(std::string username){
  int index = 0;
  for(Client c : client_db){
    if(c.username == username)
      return index;
    index++;
  }
  return -1;
}

string print_relations() {
  string relationList = "";
  for (Client c : client_db) {
    relationList += c.username + "\n"
      + "Following: ";
    for (auto it = c.client_following.begin(); it != c.client_following.end(); it++) {
      relationList += (*it)->username + " ";
    }
    relationList += "\nFollowers: ";
    for (auto it = c.client_followers.begin(); it != c.client_followers.end(); it++) {
      relationList += (*it)->username + " ";
    }
    relationList += "\n";
  }
  return relationList;
}

// Logic and data behind the server's behavior.
class MessengerServiceImpl final : public MessengerServer::Service {
  // register another server
  Status RegisterSlave(ServerContext* context, const Request* request, Reply* reply) override{
    nodeMgmt->registerSlave(request->arguments(0));
    // replies the machine's hostname
    reply->set_msg(nodeMgmt->getHostName());
    return Status::OK;
  }

  Status Sync(ServerContext* context, const SyncMsg* msg, Reply* reply) override {
    string cmd = msg->cmd();
    reply->set_msg(nodeMgmt->getHostName());
    if (cmd.compare(CMD::CHAT) == 0) {
      Message chatMsg = msg->msg();

      int user_index = find_user(chatMsg.username());
      Client* c = &client_db[user_index];

      distributeChats(c, chatMsg);
      return Status::OK;
    }
    else if (cmd.compare(CMD::DISCONN) == 0) {
      Client* c = &client_db[find_user(msg->args(0))];
      c->connected = false;
      return Status::OK;
    }
    else if (cmd.compare(CMD::JOIN) == 0) {
      Client *user1 = &client_db[find_user(msg->args(0))];
      Client *user2 = &client_db[find_user(msg->args(1))];
      user1->client_following.push_back(user2);
      user2->client_followers.push_back(user1);
#ifdef DEBUG
      cout << "After syncing join request: " << endl
        << print_relations();
#endif // DEBUG

      return Status::OK;
    }
    else if (cmd.compare(CMD::LEAVE) == 0) {

    }
    else if (cmd.compare(CMD::LOGIN) == 0) {
      Client c;
      c.username = msg->args(0);
      //c.host = msg->src();
      client_db.push_back(c);
#ifdef DEBUG
      cout << "sync login from " << msg->src() << " succeeded" << endl;
#endif // DEBUG

      return Status::OK;
    }
    else {
#ifdef DEBUG
      cout << "error in matching sync cmds" << endl;
#endif // DEBUG
      return Status::CANCELLED;
    }
  }

  //Sends the list of total rooms and joined rooms to the client
  Status List(ServerContext* context, const Request* request, ListReply* list_reply) override {
    Client user = client_db[find_user(request->username())];
    int index = 0;
    for(Client c : client_db){
      list_reply->add_all_rooms(c.username);
    }
    std::vector<Client*>::const_iterator it;
    for(it = user.client_following.begin(); it!=user.client_following.end(); it++){
      list_reply->add_joined_rooms((*it)->username);
    }
    return Status::OK;
  }

  //Sets user1 as following user2
  Status Join(ServerContext* context, const Request* request, Reply* reply) override {
    std::string username1 = request->username();
    std::string username2 = request->arguments(0);
    int join_index = find_user(username2);
    //If you try to join a non-existent client or yourself, send failure message
    if(join_index < 0 || username1 == username2)
      reply->set_msg("Join Failed -- Invalid Username");
    else{
      Client *user1 = &client_db[find_user(username1)];
      Client *user2 = &client_db[join_index];
      //If user1 is following user2, send failure message
      if(std::find(user1->client_following.begin(), user1->client_following.end(), user2) != user1->client_following.end()){
	reply->set_msg("Join Failed -- Already Following User");
        return Status::OK;
      }

      SyncMsg joinSyncMsg;
      joinSyncMsg.set_src(nodeMgmt->getHostName());
      joinSyncMsg.set_cmd(CMD::JOIN);
      joinSyncMsg.add_args(username1);
      joinSyncMsg.add_args(username2);

      nodeMgmt->sync(joinSyncMsg);

      user1->client_following.push_back(user2);
      user2->client_followers.push_back(user1);
#ifdef DEBUG
      cout << print_relations();
      // lalala
#endif // DEBUG

      reply->set_msg("Join Successful");
    }
    return Status::OK; 
  }

  //Sets user1 as no longer following user2
  Status Leave(ServerContext* context, const Request* request, Reply* reply) override {
    std::string username1 = request->username();
    std::string username2 = request->arguments(0);
    int leave_index = find_user(username2);
    //If you try to leave a non-existent client or yourself, send failure message
    if(leave_index < 0 || username1 == username2)
      reply->set_msg("Leave Failed -- Invalid Username");
    else{
      Client *user1 = &client_db[find_user(username1)];
      Client *user2 = &client_db[leave_index];
      //If user1 isn't following user2, send failure message
      if(std::find(user1->client_following.begin(), user1->client_following.end(), user2) == user1->client_following.end()){
	reply->set_msg("Leave Failed -- Not Following User");
        return Status::OK;
      }
      // find the user2 in user1 following and remove
      user1->client_following.erase(find(user1->client_following.begin(), user1->client_following.end(), user2)); 
      // find the user1 in user2 followers and remove
      user2->client_followers.erase(find(user2->client_followers.begin(), user2->client_followers.end(), user1));
      reply->set_msg("Leave Successful");
    }
    return Status::OK;
  }

  //Called when the client startd and checks whether their username is taken or not
  Status Login(ServerContext* context, const Request* request, Reply* reply) override {
    Client c;
    std::string username = request->username();
    int user_index = find_user(username);
    if(user_index < 0){
      // set up username and hostname
      c.username = username;
      //c.host = nodeMgmt->getHostName();
      c.onServer = true;
      client_db.push_back(c);
      // sync login with other servers
      SyncMsg msg;
      msg.set_src(nodeMgmt->getHostName());
      msg.set_cmd(CMD::LOGIN);
      msg.add_args(username);
      nodeMgmt->sync(msg);

      reply->set_msg("Login Successful!");
#ifdef DEBUG
      cout << username << " Login Successful!" << endl;
#endif
    }
    else{ 
      Client *user = &client_db[user_index];
      if(user->connected)
        reply->set_msg("Invalid Username");
      else{
        std::string msg = "Welcome Back " + user->username;
	reply->set_msg(msg);
        user->connected = true;
      }
    }
    return Status::OK;
  }

  Status Chat(ServerContext* context,
    ServerReaderWriter<Message, Message>* stream) override {
    Message message;
    Client *c;
    //Read messages until the client disconnects
    while (stream->Read(&message)) {
#ifdef DEBUG
      cout << "=============receive message: " << message.msg() << endl;
#endif // DEBUG

      // get username that sent the chat
      std::string username = message.username();
      // find the client index
      int user_index = find_user(username);
      // get the client pointer
      c = &client_db[user_index];
      
      //"Set Stream" is the default message from the client to initialize the stream
      if (message.msg() != "Set Stream"){
        distributeChats(c, message);
#ifdef DEBUG
        cout << "after distributing chats on the original server" << endl;
#endif // DEBUG

        // after the function, all files should be written but only local
        // followers will already receive chat update
        // now send updates to other servers
        // construct sync msg
        SyncMsg chatSync;
        chatSync.set_src(nodeMgmt->getHostName());
        chatSync.set_cmd(CMD::CHAT);
        chatSync.set_allocated_msg(&message);
        // sync the chats on other servers
        nodeMgmt->sync(chatSync);
        chatSync.release_msg();
      }
      //If message = "Set Stream", print the first 20 chats from the people you follow
      else {
#ifdef DEBUG
        cout << "============Setup streams" << endl;
#endif // DEBUG

        if (c->stream == 0)
          c->stream = stream;
        std::string line;
        std::vector<std::string> newest_twenty;
        std::ifstream in(username + "following.txt");
        int count = 0;
        //Read the last up-to-20 lines (newest 20 messages) from userfollowing.txt
        while (getline(in, line)) {
          if (c->following_file_size > 20) {
            if (count < c->following_file_size - 20) {
              count++;
              continue;
            }
          }
          newest_twenty.push_back(line);
        }
        Message new_msg;
        //Send the newest messages to the client to be displayed
        for (int i = 0; i<newest_twenty.size(); i++) {
          new_msg.set_msg(newest_twenty[i]);
          stream->Write(new_msg);
        }
        continue;
      }
    }
    //If the client disconnected from Chat Mode, set connected to false
    SyncMsg msg;
    msg.set_src(nodeMgmt->getHostName());
    msg.set_cmd(CMD::DISCONN);
    msg.add_args(c->username);
    nodeMgmt->sync(msg);

    c->connected = false;
    return Status::OK;
  }

};

void RunChatServer(string hostname, string port_no, string mServerInfo) {
  nodeMgmt = new NodeMgmt((hostname + ":" + port_no), mServerInfo);

  std::string server_address = "0.0.0.0:"+port_no;
  MessengerServiceImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Chat Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

void RunMasterServer(string hostname, string m_port) {
  masterMgmt = new MasterMgmt(hostname + ":" + m_port);

  string mserver_addr = "0.0.0.0:" + m_port;
  MasterServer m_service;
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(mserver_addr, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&m_service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Master Server listening on " << mserver_addr << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  //ifstream serverFile("./servers.txt");
  //string mServerInfo;

  char hostname[128];
  gethostname(hostname, sizeof (hostname));
  strcat(hostname, ".cse.tamu.edu");

  /*if (serverFile.is_open()) {
    getline(serverFile, mServerInfo);
    while (getline(serverFile, line)) {
      if(!isSameHost(line, hostname))
        servers.push_back(line);
    }
    serverFile.close();
  }*/
  
  std::string port = "10010", m_port = "10009", m_addr = "lenss-comp1.cse.tamu.edu:10009";
  int opt = 0;
  bool isMaster = true;
  while ((opt = getopt(argc, argv, "p:m:")) != -1){
    switch(opt) {
      case 'p':
          port = optarg;
          break;
      case 'm':
        isMaster = false;
        m_addr = optarg;
        break;
      default:
	  std::cerr << "Invalid Command Line Argument\n";
    }
  }

  if (isMaster) {
    RunMasterServer(string(hostname), m_port);
  }
  else
    RunChatServer(string(hostname), port, m_addr);

  return 0;
}
