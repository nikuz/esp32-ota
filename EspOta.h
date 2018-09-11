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
    unsigned long int getUpdateTime();
    void begin(unsigned long int timestamp);

   private:
    void getCurrentETag();
    void validate();
    void update(unsigned long int timestamp);
    void end();
};

#endif
