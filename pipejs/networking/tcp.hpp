
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

    static void echoRead(uv_stream_t *server, ssize_t nread, const uv_buf_t* buf) {
      if (nread == -1) {
          fprintf(stderr, "error echoRead");
          return;
      }

      printf("result: %s\n", buf->base);
    }

    static void onWriteEnd(uv_write_t *req, int status) {
      if (status == -1) {
        fprintf(stderr, "error onWriteEnd");
        return;
      }
      uv_read_start(req->handle, Tcp::onAllocCallback, Tcp::echoRead);
    }

    static void writeToSocket(Client* c, char* buf, size_t len) {
      if (len <= 0) {
        return;
      }

      uv_buf_t uvbuf;
      uvbuf.base = buf;
      uvbuf.len = len;
  
      int r = uv_write(&c->writeReq, (uv_stream_t*)&c->socket, &uvbuf, 1, Tcp::onWriteEnd);
      if (r < 0) {
        printf("ERROR: writeToSocket error: ");
      }
    }

    static void flushReadBio(Client* c) {
      char buf[5000*16]; // TODO: check this values before
      int bytesRead = 0;
      while((bytesRead = BIO_read(c->writeBio, buf, sizeof(buf))) > 0) {
        Tcp::writeToSocket(c, buf, bytesRead);
      }
    }

    static void checkOutgoingApplicationData(Client* c) {
      if (SSL_is_init_finished(c->ssl)) {
        if (c->bufferOut.size() > 0) {
          std::copy(c->bufferOut.begin(), c->bufferOut.end(), std::ostream_iterator<char>(std::cout,""));
          int sslWriteResponse = SSL_write(c->ssl, &c->bufferOut[0], c->bufferOut.size());
          c->bufferOut.clear();
          Tcp::flushReadBio(c);
        }
      }
    }

    static void handleError(Client* c, int result) {
      int error = SSL_get_error(c->ssl, result);

      if (error == SSL_ERROR_WANT_READ) { // wants to read from bio
        Tcp::flushReadBio(c);
      }
      if (error == SSL_ERROR_SSL) {
        printf("## SSL Error %d \n", error);
      }
    }

    static void onSocketEvent(Client* c) {
      char buf[5000 * 10]; // TODO: check this values before
      int bytesRead = 0;

      if (!SSL_is_init_finished(c->ssl)) {
        int sslConnectResponse = SSL_connect(c->ssl);
        if (sslConnectResponse < 0) {
          Tcp::handleError(c, sslConnectResponse); // TODO: Improve error handle
          printf("ERROR: init on_event 1 %d \n", sslConnectResponse);
        }

        Tcp::checkOutgoingApplicationData(c);
      } else {
        // connect, check if there is encrypted data, or we need to send app data
        int sslReadResponse = SSL_read(c->ssl, buf, sizeof(buf));
        if (sslReadResponse < 0) {
          printf("ERROR: init onSocketEvent 2 \n");
        } else if (sslReadResponse > 0) {
          std::copy(buf, buf+sslReadResponse, std::back_inserter(c->bufferIn));
          std::copy(c->bufferIn.begin(), c->bufferIn.end(), std::ostream_iterator<char>(std::cout));
          c->bufferIn.clear();
        }

        Tcp::checkOutgoingApplicationData(c);
      }
    }

    static void onReadCallback(uv_stream_t *server, ssize_t nread, const uv_buf_t* buf) {
      Client* c = static_cast<Client*>(server->data);
      if (nread == -1) {
        printf("ERROR: on_read_callback");
      }

      BIO_write(c->readBio, buf->base, nread);
      Tcp::onSocketEvent(c);
    }

    static void onConnectCallback(uv_connect_t* con, int status) {
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

      // TODO: Try putting this in another class
      const char* httpRequestTemplate = "" \
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: PostmanRuntime/7.36.1\r\n"
        "Accept: */*\r\n"
        "Connection: close\r\n"
        "\r\n";

      char httpRequest[5000];
      sprintf(httpRequest, httpRequestTemplate, c->page, c->host);
      c->addAppData(httpRequest);
      c->ssl = SSL_new(c->sslCTX);

      std::string servername = c->host;
      SSL_ctrl(c->ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME, TLSEXT_NAMETYPE_host_name, (void*)servername.c_str());

      c->readBio = BIO_new(BIO_s_mem());
      c->writeBio = BIO_new(BIO_s_mem());
      SSL_set_bio(c->ssl, c->readBio, c->writeBio);
      SSL_set_connect_state(c->ssl);

      Tcp::onSocketEvent(c);
    }

    static void onResolvedCallback(uv_getaddrinfo_t* resolver, int status, struct addrinfo * res) {
      Client* c = static_cast<Client*>(resolver->data);
      if (status == -1) {
        printf("ERROR: getaddrinfo callback error \n");
        ::exit(0);
      }

      char addr[17] = {'\0'};
      uv_ip4_name((struct sockaddr_in*) res->ai_addr, addr, 16);
      // printf("Found host:  %s\n", addr);

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