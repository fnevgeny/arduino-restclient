#include "RestClient.h"

#ifndef HTTP_DEFAULT_CONTENT_TYPE
#  define HTTP_DEFAULT_CONTENT_TYPE "application/json"
#endif

#ifdef HTTP_DEBUG
#  define HTTP_DEBUG_PRINT(string) (Serial.print(string))
#else
#  define HTTP_DEBUG_PRINT(string)
#endif

RestClient::RestClient(EthernetClient *_client){
  client = _client;
  host = NULL;
  hostIp = {0, 0, 0, 0};
  port = 80;
  num_headers = 0;
  contentType = HTTP_DEFAULT_CONTENT_TYPE;
  maxResponseLength = (unsigned int) (-1);
  responseTimeout = 300;
}

// Set server attributes
void RestClient::setServer(const char* _host) {
  host = _host;
  port = 80;
}

void RestClient::setServer(const char* _host, int _port) {
  host = _host;
  port = _port;
}

void RestClient::setServer(IPAddress ip, int _port) {
  host = NULL;
  hostIp = ip;
  port = _port;
}

void RestClient::setMaxResponseLength(unsigned int length) {
  maxResponseLength = length;
}

void RestClient::setResponseTimeout(unsigned int timeout) {
  responseTimeout = timeout;
}

// GET path
int RestClient::get(const char* path){
  return request("GET", path, NULL, NULL);
}

//GET path with response
int RestClient::get(const char* path, String* response){
  return request("GET", path, NULL, response);
}

// POST path and body
int RestClient::post(const char* path, const char* body){
  return request("POST", path, body, NULL);
}

// POST path and body with response
int RestClient::post(const char* path, const char* body, String* response){
  return request("POST", path, body, response);
}

// PUT path and body
int RestClient::put(const char* path, const char* body){
  return request("PUT", path, body, NULL);
}

// PUT path and body with response
int RestClient::put(const char* path, const char* body, String* response){
  return request("PUT", path, body, response);
}

// DELETE path
int RestClient::del(const char* path){
  return request("DELETE", path, NULL, NULL);
}

// DELETE path and response
int RestClient::del(const char* path, String* response){
  return request("DELETE", path, NULL, response);
}

// DELETE path and body
int RestClient::del(const char* path, const char* body ){
  return request("DELETE", path, body, NULL);
}

// DELETE path and body with response
int RestClient::del(const char* path, const char* body, String* response){
  return request("DELETE", path, body, response);
}

void RestClient::write(const char* string){
  HTTP_DEBUG_PRINT(string);
  client->print(string);
}

void RestClient::setHeader(const char* header){
  headers[num_headers] = header;
  num_headers++;
}

void RestClient::setContentType(const char* contentTypeValue){
  contentType = contentTypeValue;
}

// The mother- generic request method.
//
int RestClient::request(const char* method, const char* path,
                  const char* body, String* response){

  HTTP_DEBUG_PRINT(F("HTTP: connect\n"));

  byte status;
  if (host != NULL) {
    status = client->connect(host, port);
  } else {
    status = client->connect(hostIp, port);
  }
  if (status == 1) {
    HTTP_DEBUG_PRINT(F("HTTP: connected\n"));
    HTTP_DEBUG_PRINT(F("REQUEST: \n"));
    // Make a HTTP request line:
    write(method);
    write(" ");
    write(path);
    write(" HTTP/1.1\r\n");
    for(int i=0; i<num_headers; i++){
      write(headers[i]);
      write("\r\n");
    }
    write("Host: ");
    write(host);
    write("\r\n");
    write("Connection: close\r\n");

    if(body != NULL){
      char contentLength[30];
      sprintf(contentLength, "Content-Length: %d\r\n", strlen(body));
      write(contentLength);

      write("Content-Type: ");
      write(contentType);
      write("\r\n");
    }

    write("\r\n");

    if(body != NULL){
      write(body);
      write("\r\n");
      write("\r\n");
    }

    // make sure everything is written
    client->flush();

    HTTP_DEBUG_PRINT(F("HTTP: call readResponse\n"));
    int statusCode = readResponse(response);
    HTTP_DEBUG_PRINT(F("HTTP: return readResponse\n"));

    //cleanup
    HTTP_DEBUG_PRINT(F("HTTP: stop client\n"));
    num_headers = 0;
    client->stop();
    HTTP_DEBUG_PRINT(F("HTTP: client stopped\n"));

    return statusCode;
  }else{
    HTTP_DEBUG_PRINT(F("HTTP Connection failed\n"));
    return 0;
  }
}

int RestClient::readResponse(String* response) {

  // an http request ends with a blank line
  boolean currentLineIsBlank = true;
  boolean httpBody = false;
  boolean inStatus = false;

  char statusCode[4];
  int i = 0;
  int code = 0;

  unsigned long millis_start;

  if(response == NULL){
    HTTP_DEBUG_PRINT(F("HTTP: NULL RESPONSE POINTER: \n"));
  }else{
    HTTP_DEBUG_PRINT(F("HTTP: NON-NULL RESPONSE POINTER: \n"));
  }

  HTTP_DEBUG_PRINT(F("HTTP: RESPONSE: \n"));
  millis_start = millis();
  while (client->connected() && millis() < millis_start + 1000L*responseTimeout) {
    HTTP_DEBUG_PRINT(F("."));

    if (client->available()) {
      HTTP_DEBUG_PRINT(F(","));

      char c = client->read();
      HTTP_DEBUG_PRINT(c);

      if(c == ' ' && !inStatus){
        inStatus = true;
      }

      if(inStatus && i < 3 && c != ' '){
        statusCode[i] = c;
        i++;
      }
      if(i == 3){
        statusCode[i] = '\0';
        code = atoi(statusCode);
      }

      if(httpBody){
        if(response != NULL && response->length() < maxResponseLength) {
          response->concat(c);
        } else {
          break;
        }
      }
      else
      {
          if (c == '\n' && currentLineIsBlank) {
            httpBody = true;
            if (response == NULL) {
              // nothing interesting for us will follow
              break;
            }
          }

          if (c == '\n') {
            // you're starting a new line
            currentLineIsBlank = true;
          }
          else if (c != '\r') {
            // you've gotten a character on the current line
            currentLineIsBlank = false;
          }
      }
    }
  }

  HTTP_DEBUG_PRINT(F("HTTP: return readResponse3\n"));
  return code;
}
