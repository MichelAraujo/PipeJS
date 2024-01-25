
struct Client {
  void addAppData(const std::string str) {
    std::copy(str.begin(), str.end(), std::back_inserter(bufferOut));
  }

  uv_loop_t* loop;
  uv_tcp_t socket;
  uv_write_t writeReq;
  uv_connect_t connectReq;
  char host[256];
  char port[4];
  char page[256];
  std::vector<char> bufferIn;
  std::vector<char> bufferOut;
  SSL_CTX* sslCTX;
  SSL* ssl;
  BIO* readBio;
  BIO* writeBio;
};

class Tcp {
  private:
    Client client;

    static void onAllocCallback(uv_handle_t* handle, size_t suggestedSize, uv_buf_t* buf) {
      buf->base = (char*)malloc(suggestedSize);
      buf->len = suggestedSize;
    }

    static void onReadCallback(uv_stream_t *server, ssize_t nread, const uv_buf_t* buf) {
      Client* c = static_cast<Client*>(server->data);
      if (nread == -1) {
        printf("ERROR: on_read_callback");
      }
    }

    static void onConnectCallback(uv_connect_t* con, int status) {
      printf("### On Connect CB ### \n");
      Client* c = static_cast<Client*>(con->data);
      if (status == -1) {
        printf("ERROR: on_connect_callback \n");
        ::exit(0);
      }

      int r = uv_read_start((uv_stream_t*)&c->socket, Tcp::onAllocCallback, Tcp::onReadCallback);
      if (r == -1) {
        printf("ERROR: uv_read_start \n");
        ::exit(0);
      }
    }

    static void onResolvedCallback(uv_getaddrinfo_t* resolver, int status, struct addrinfo * res) {
      Client* c = static_cast<Client*>(resolver->data);
      if (status == -1) {
        printf("ERROR: getaddrinfo callback error \n");
        ::exit(0);
      }

      char addr[17] = {'\0'};
      uv_ip4_name((struct sockaddr_in*) res->ai_addr, addr, 16);
      printf("Found host:  %s\n", addr);

      uv_tcp_init(c->loop, &c->socket);
      uv_tcp_connect(&c->connectReq, &c->socket, (const struct sockaddr*)res->ai_addr, Tcp::onConnectCallback);
      uv_freeaddrinfo(res);
    }

    void resolveHost() {
      struct addrinfo hints;
      hints.ai_family = PF_INET;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_protocol = IPPROTO_TCP;
      hints.ai_flags = 0;

      uv_getaddrinfo_t resolver;
      resolver.data = &this->client;
      uv_getaddrinfo(
        this->client.loop,
        &resolver,
        Tcp::onResolvedCallback,
        this->client.host,
        this->client.port,
        &hints
      );

      uv_run(this->client.loop, UV_RUN_DEFAULT);
    }

  public:
    void connect(uv_loop_t* loop, SSLHandler* sslHandler) {
      this->client.loop = loop;
      this->client.connectReq.data = &client;
      this->client.socket.data = &client;
      this->client.ssl = NULL;
      this->client.sslCTX = sslHandler->getCTX();

      sprintf(this->client.host, "%s", "seguro.catho.com.br");
      sprintf(this->client.port, "%s", "443");
      sprintf(this->client.page, "%s", "/vagas/vagas-api/role/?keywords=pedreiro");

      this->resolveHost();
    }
};