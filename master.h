#pragma once
#ifndef MASTER_H
#define MASTER_H

#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>
#include "fb.grpc.pb.h"
#include <string>
#include <vector>

using grpc::ServerContext;
using hw2::Request;
using hw2::Reply;
using hw2::Master;
using grpc::Status;

using namespace std;

// round robin term
int rr_term = 0;
// node servers
vector<string> servers;

class MasterServer final : public Master::Service {
  Status RequestServer(ServerContext* context, const Request* request, Reply* reply) {
    string server = servers[rr_term];
    reply->set_msg(server);
    rr_term = (++ rr_term) % servers.size();
    return Status::OK;
  }
};

#endif // !MASTER_H
