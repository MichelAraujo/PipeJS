
class SSLHandler {
  private:
    SSL_CTX* sslCTX;

  public:
    void init() {
      SSL_library_init();
      SSL_load_error_strings();

      // For now works only for TCP Client (GET)
      this->sslCTX = SSL_CTX_new(TLS_client_method());
    }

    SSL_CTX* getCTX() {
      return this->sslCTX;
    }
};