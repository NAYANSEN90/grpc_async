#include <iostream>
#include <utility>
#include <grpcpp/grpcpp.h>
#include "proto/rpc.grpc.pb.h"

class RPCHandler {
 public:
  virtual ~RPCHandler() = default;
  virtual void Proceed() = 0;
};

class RPC1Handler : public RPCHandler {
 public:
  RPC1Handler(rpc::SampleSvc::AsyncService* service, grpc::ServerCompletionQueue* cq) :
    service_(service), cq_(cq), responder_(&ctx_), done_(false) {
    Register();
  }

  ~RPC1Handler() override = default;

 public:
  void Proceed() override {
    if (!done_) {
      std::cout << "Inside Proceed RPC1" << std::endl;
      new RPC1Handler(this->service_, this->cq_);
      done_.store(true);
      responder_.Finish(response_, grpc::Status::OK, this);
    } else {
      Cleanup();
    }
  }
 private:
  void Register() {
    std::cout << "Inside Register RPC1" << std::endl;
    service_->RequestRPC_1(&ctx_, &request_, &responder_, cq_, cq_, this);
  }
  void Cleanup() {
    std::cout << "Inside Cleanup RPC1" << std::endl;
    delete this;
  }
 private:
  rpc::SampleSvc::AsyncService* service_;
  grpc::ServerCompletionQueue* cq_;
  grpc::ServerContext ctx_;
  rpc::RPC1Request request_;
  rpc::RPC1Response response_;
  grpc::ServerAsyncResponseWriter<rpc::RPC1Response> responder_;
  std::atomic_bool done_;
};

class RPC2Handler : public RPCHandler {
 public:
  RPC2Handler(rpc::SampleSvc::AsyncService* service, grpc::ServerCompletionQueue* cq) :
    service_(service), cq_(cq), responder_(&ctx_), done_(false) {
    Register();
  }

  ~RPC2Handler() override = default;

 public:
  void Proceed() override {
    if (!done_) {
      std::cout << "Inside Proceed RPC2" << std::endl;
      new RPC2Handler(this->service_, this->cq_);
      done_.store(true);
      responder_.Finish(response_, grpc::Status::OK, this);
    } else {
      Cleanup();
    }
  }
 private:
  void Register() {
    std::cout << "Inside Register RPC2" << std::endl;
    service_->RequestRPC_2(&ctx_, &request_, &responder_, cq_, cq_, this);
  }
  void Cleanup() {
    std::cout << "Inside Cleanup RPC2" << std::endl;
    delete this;
  }
 private:
  rpc::SampleSvc::AsyncService* service_;
  grpc::ServerCompletionQueue* cq_;
  grpc::ServerContext ctx_;
  rpc::RPC2Request request_;
  rpc::RPC2Response response_;
  grpc::ServerAsyncResponseWriter<rpc::RPC2Response> responder_;
  std::atomic_bool done_;
};

class Server {
 public:
  explicit Server(std::string addr) : addr_(std::move(addr)) {
    // Empty
  }

  ~Server() {
    server_->Shutdown();
    cq_->Shutdown();
  }

  void Start() {
    grpc::ServerBuilder builder;
    builder.AddListeningPort(addr_, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    cq_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();
    Install();
    Loop();
  }

 private:
  void Install() {
    new RPC1Handler(&this->service_, this->cq_.get());
    new RPC2Handler(&this->service_, this->cq_.get());
  }

  [[noreturn]]
  void Loop() {
    void* tag;
    bool ok;
    while(true) {
      if (GPR_UNLIKELY(!(cq_->Next(&tag, &ok)))) {
        continue;
      }
      if (GPR_UNLIKELY(!(ok))) {
        continue;
      }
      static_cast<RPCHandler*>(tag)->Proceed();
    }
  }

 private:
  std::string addr_;
  std::unique_ptr<grpc::Server> server_;
  rpc::SampleSvc::AsyncService service_;
  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
};

int main() {
  std::cout << "Hello, World!" << std::endl;

  Server server("0.0.0.0:12345");
  server.Start();
  return 0;
}
