#include <fstream>
#include <iostream>
#include <memory>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <ctime>
#include <thread>

#include "utils.h"

#include "fb.grpc.pb.h"
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>
#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>


using google::protobuf::Timestamp;
using google::protobuf::Duration;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;

using grpc::Status;

using grpc::ClientContext;

using hw2::Message;
using hw2::ListReply;
using hw2::Request;
using hw2::Reply;
using hw2::MessengerServer;
using hw2::Master;
using hw2::NodeReq;

using namespace std;

class CMD {
public:
  static const string JOIN;
  static const string LOGIN;
  static const string LEAVE;
  static const string CHAT;
};

const string CMD::JOIN = "JOIN";
const string CMD::LOGIN = "LOGIN";
const string CMD::LEAVE = "LEAVE";
const string CMD::CHAT = "CHAT";
#pragma once
