#ifndef _TCP_H
#define _TCP_H

#include "buffer.h"
#include <ScktComp.hpp>
#include "CriticalLock.h"
#include <fstream>

namespace TCP{
	//client connection status
	enum CONNECTION_STATUS{
		CONNECTION_NOT_STARTED,
		CONNECTION_WAITING,
		CONNECTION_CONNECTED,
		CONNECTION_ERROR,
		CONNECTION_CLOSING,
		CONNECTION_DISCONNECTED
	};
	
	//chars used to construct the message
	const char HEAD_START = 1;
	const char TEXT_START = 2;
	const char END_TEXT = 3;
	const char END_TRANS = 4;
	
	//check a stream buffer for a message.
	//If found populate the message vector with the data, and remove it from the stream buffer
	template<typename stream_type>
	bool GetMessageFromStreamBuffer(StreamBuffer<stream_type>& stream, std::vector<unsigned char>& message);
	
	//Write the data and the message header to the given stream
	// SOH <4-byte-data-length> STX <data-length-bytes> ETX EOT
	template<typename stream_type>
	void WriteMessageToStreamBuffer(StreamBuffer<stream_type>& stream, const unsigned char * buffer, const unsigned int& length);
	
	//Base class for client and server client
    class client_base{
	protected:
		//temp buffer for reading from the socket
		std::vector<unsigned char> tmpInBuff;
		int tmpInSz;
		
	public:
		//the actual data
		StreamBuffer<unsigned char> outstream;
		StreamBuffer<unsigned char> instream;
		
		//locks
		CRITICAL_SECTION in_stream_lock, out_stream_lock;
		
		~client_base(){
			//cleanup the critical sections
			LeaveCriticalSection(&in_stream_lock);
			LeaveCriticalSection(&out_stream_lock);
			
			DeleteCriticalSection(&in_stream_lock);
			DeleteCriticalSection(&out_stream_lock);
		}
		
		client_base(){
			//initialize the critical sections
			InitializeCriticalSection(&in_stream_lock);
			InitializeCriticalSection(&out_stream_lock);
		}
		
		//send data in the outstream to the socket
		template<typename socket_type>
		bool sendOut(socket_type* socket);
		
		//write data to the outstream, and send if the stream was empty
		template<typename socket_type>
		bool send(socket_type* socket, const unsigned char * buffer, const unsigned int& length);
		
		//read data from the socket into the instream
		template<typename socket_type>
		void readSocket(socket_type* socket);
		
		//returns true or false if there is a message to get
		//if there is a message, the vector "message" is cleared, and the message data is inserted into it.
		bool getMessage(std::vector<unsigned char>& message){
			CRTLK(in_stream_lock);
			return GetMessageFromStreamBuffer(instream, message);
		}
	};
	
	//TCP Client class
	class client : protected client_base{
	public:
		typedef void (__closure* Event)(client* client);
		typedef void (__closure* ErrorEvent)(client* client, TErrorEvent ev, int& ErrorCode);
	protected:
		//actual socket
		TClientSocket* _socket;
		
		//connection info
		AnsiString _addr;
		int _prt;
		
		CONNECTION_STATUS _constat;
		AnsiString _lastException;
		
		//initialize a new socket
		void createNewSocket();
		
		//events
		virtual void __fastcall _onconnect(TObject* Sender, TCustomWinSocket *Socket);
		virtual void __fastcall _ondisconnect(TObject* Sender, TCustomWinSocket *Socket);
		virtual void __fastcall _onread(TObject* Sender, TCustomWinSocket* Socket);
		virtual void __fastcall _onwrite(TObject* Sender, TCustomWinSocket* Socket);
		virtual void __fastcall _onerror(TObject* Sender, TCustomWinSocket* Socket, TErrorEvent ev, int& ErrorCode);
		
	public:
		
		~client();
		client();
		client(const AnsiString& address, const int& port = -1);
		
		//get connection info
		const AnsiString& Address() const{	return _addr;	}
		AnsiString& Address(){	return _addr;	}
		const int& Port() const{	return _prt;	}
		int& Port(){	return _prt;	}
		
		CONNECTION_STATUS connectionStatus() const{	return _constat;	}
		const AnsiString& getLastException() const{	return _lastException;	}
		
		//connect to a socket
		//returns false if already connected, or waiting for one
		bool connect();
		//connect with new info
		bool connect(const AnsiString& address, int port);
		
		//close the current connection
		//returns false if not connected
		bool disconnect();

		//send data to the socket
		//returns false if not connected or the send failed
		bool send(const unsigned char * buffer, const unsigned int& length);
		
		//retreive a message from the input buffer
		bool getMessage(std::vector<unsigned char>& message){
			return client_base::getMessage(message);
		}
		
		//events
		Event OnConnect;
		Event OnFailedConnect;
		Event OnDisconnect;
		Event OnRead;
		//If OnError is not set, the connection will attempt to close on any error
		ErrorEvent OnError;
	};
	
	//the client connection used by the server
	class serverClientSocket : public TServerClientWinSocket{
	public:
		//the data, and methods for sending/receiving it
		client_base _data;
		
		//the constructor must have these parameters and call the TServerClientWinSocket constructor
		__fastcall serverClientSocket(int socket, TServerWinSocket* serverWinSocket):
			TServerClientWinSocket(socket, serverWinSocket){
		}
		
		//send data to the socket
		//returns false if the sed command failed
		bool send(const unsigned char * buffer, const unsigned int& length);
		
		//retreive a message from the input buffer
		bool getMessage(std::vector<unsigned char>& message){
			return _data.getMessage(message);
		}
	};
	
	//TCP Server class
	class server{
	public:
		typedef void (__closure* clientEvent)(serverClientSocket* client);
		typedef void (__closure* clientErrorEvent)(serverClientSocket* client, TErrorEvent ev, int& ErrorCode);
		
	protected:
		//the actual socket
		TServerSocket* _socket;
		
		//the port
		int _prt;
	
		//client events
		virtual void __fastcall _ongetclientsocket(TObject * Sender, int socket, TServerClientWinSocket* &ClientSocket);
		virtual void __fastcall _onclientconnect(TObject* Sender, TCustomWinSocket *Socket);
		virtual void __fastcall _onclientdisconnect(TObject* Sender, TCustomWinSocket *Socket);
		virtual void __fastcall _onclientread(TObject* Sender, TCustomWinSocket* Socket);
		virtual void __fastcall _onclientwrite(TObject* Sender, TCustomWinSocket* Socket);
		virtual void __fastcall _onclienterror(TObject* Sender, TCustomWinSocket* Socket, TErrorEvent ev, int& ErrorCode);
		virtual void __fastcall _OnError(TObject * Sender, TCustomWinSocket * Socket, TErrorEvent ErrorEvent, int &ErrorCode);
		
	public:
	
		~server();
		server(int port = -1);
	
		//get/set the port
		const int& Port() const{	return _prt;	}
		int& Port(){	return _prt;	}
		
		//host name
		AnsiString getHostname(){	return (_socket != NULL)?_socket->Socket->LocalHost:AnsiString("<NULL>");	}
		
		//check the socket
		bool isListening(){	return _socket != NULL;	}
		
		//start listening
		bool listen();
		bool listen(int port);
		
		//stop the server
		bool stop();
	
		//the number of connected clients
		int numberOfConnections(){	return _socket==NULL?-1:_socket->Socket->ActiveConnections;	}
		//retreive a specific client by position in connection index
		serverClientSocket* getClient(int at);
		//retrieve a specific client by ip address
		serverClientSocket* getClientByIP(const AnsiString& ipAddress);
		//check if a specific client has a message
		bool getMessageFromClient(int at, std::vector<unsigned char>& message);
		
		//send a message to all connected clients
		int sendToAll(const unsigned char * buffer, const unsigned int& length);
		
		//client events
		clientEvent OnClientConnect;
		clientEvent OnClientDisconnect;
		clientEvent OnClientRead;
		//If OnClientError is not set, the connection will attempt to close on any error
		clientErrorEvent OnClientError;
		
		clientEvent OnClientCreated;
		
		//server error event
		void (__closure* OnError)(TErrorEvent ErrorEvent, int &ErrorCode);
	};
}; //end namespace TCP

#endif //_TCP_H
