#ifndef EspOta_h
#define EspOta_h

#include "Arduino.h"

class EspOta {
   public:
    EspOta(const String host, const int port, const String bin, const char *pKey);
    ~EspOta();

    String _host;
    int _port;
    String _bin;

    void updateEntries(const String host, const int port, const String bin);
    void begin(String timestamp);
    static String getUpdateTime();

   private:
    void getCurrentETag();
    void validate();
    void update(String timestamp);
    void end();
};

#endif
