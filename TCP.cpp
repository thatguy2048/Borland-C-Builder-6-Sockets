#include "TCP.h"

namespace TCP{
//--------------------------------Message-Processing----------------------------------//
	template<typename stream_type>
	bool GetMessageFromStreamBuffer(StreamBuffer<stream_type>& stream, std::vector<unsigned char>& message){
		bool output = false;
		StreamBuffer<stream_type>::iterator fnd = stream.find(HEAD_START);
		while(fnd != stream.end()){
			StreamBuffer<stream_type>::iterator fnd_start = fnd+5;
			if(*fnd_start == TEXT_START){ //length found
				unsigned int messageLength = *((unsigned int*)(fnd+1));
				if(stream.length() < messageLength+7+(fnd-stream.begin())){
					//the stream is not long enough to contain the data
					break;
				}else{
					StreamBuffer<stream_type>::iterator fnd_end = fnd_start+messageLength+1;
					if(*fnd_end == END_TEXT && *(fnd_end+1) == END_TRANS){//found a complete message
						message.insert(message.begin(), fnd_start+1, fnd_end);
						output = true;
						stream.erase_until(fnd_end+2);
						break;
					}else{//end not in the correct position, false start
						stream.erase_until(fnd+1);
						fnd = stream.find(HEAD_START);
					}
				}
			}else{ //length not found, false start
				stream.erase_until(fnd+1);
				fnd = stream.find(HEAD_START);
			}
		}
		return output;
	}
	
	template<typename stream_type>
	void WriteMessageToStreamBuffer(StreamBuffer<stream_type>& stream, const unsigned char * buffer, const unsigned int& length){
		stream.write(HEAD_START);
		stream.write((const unsigned char*)&length,4);
		stream.write(TEXT_START);
		stream.write(buffer,length);
		stream.write(END_TEXT);
		stream.write(END_TRANS);
	}
//----------------------------------client_base---------------------------------------//
	template<typename socket_type>
	bool client_base::sendOut(socket_type* socket){
		int sent = -1;
		if(!outstream.empty()){
			try{
				sent = socket->SendBuf(outstream.begin(),outstream.length());
			}catch(...){
				sent = -1;
			}
			if(sent > 0){
				outstream.erase(sent);
			}
		}
		return sent > 0;
	}
	
	template<typename socket_type>
	bool client_base::send(socket_type* socket, const unsigned char * buffer, const unsigned int& length){
		CRTLK(out_stream_lock);
		bool tosend = outstream.empty();
		WriteMessageToStreamBuffer(outstream, buffer, length);
		return tosend?sendOut(socket):true;
	}
	
	template<typename socket_type>
	void client_base::readSocket(socket_type* socket){
		tmpInSz = socket->ReceiveLength();
		if(tmpInSz > 0){
			CRTLK(in_stream_lock);
			tmpInBuff.reserve(tmpInSz);
			tmpInSz = socket->ReceiveBuf(tmpInBuff.begin(), tmpInSz);
			instream.write(tmpInBuff.begin(), tmpInSz);
		}
	}

//----------------------------------client---------------------------------------//
	client::~client(){
		if(_socket != NULL){
			__try{
				delete _socket;
			}__except(EXCEPTION_EXECUTE_HANDLER){}
		}
	}
	
	client::client()
		:_prt(-1), _socket(NULL),OnConnect(NULL), 
		OnDisconnect(NULL), OnRead(NULL), OnFailedConnect(NULL),
		_constat(CONNECTION_NOT_STARTED){	
		
	}
	
	client::client(const AnsiString& address, const int& port)
		:_addr(address), _prt(port), _socket(NULL), 
		OnConnect(NULL), OnDisconnect(NULL), OnRead(NULL),
		_constat(CONNECTION_NOT_STARTED){
		
	}
	

	void client::createNewSocket(){
		if(_socket != NULL){
			__try{
				delete _socket;
			}__except(EXCEPTION_EXECUTE_HANDLER){}
		}
		_socket = new TClientSocket(NULL);
		_socket->Address = _addr;
		_socket->Port = _prt;
		
		_socket->OnConnect = _onconnect;
		_socket->OnDisconnect = _ondisconnect;
		_socket->OnWrite = _onwrite;
		_socket->OnRead = _onread;
		_socket->OnError = _onerror;
		
		outstream.clear();
		instream.clear();
	}
	
	bool client::connect(){
		bool output = false;
		if(_constat == CONNECTION_CONNECTED || _constat == CONNECTION_WAITING){
		}else{
			createNewSocket();
			try{
				_socket->Open();
				_constat = CONNECTION_WAITING;
			}catch(const Exception& e){
				_lastException = e.Message;
				_constat = CONNECTION_ERROR;
			}
			output = (_constat == CONNECTION_WAITING);
		}
		return output;
	}
	
	bool client::connect(const AnsiString& address, int port){
		_addr = address;
		_prt = port;
		return connect();
	}
	
	bool client::disconnect(){
		bool output = false;
		if(_socket == NULL || _constat == CONNECTION_NOT_STARTED || _constat == CONNECTION_CLOSING || _constat == CONNECTION_DISCONNECTED){
		
		}else{
			try{
				_socket->Close();
				_constat = CONNECTION_CLOSING;
			}catch(const Exception& e){
				_lastException = e.Message;
				_constat = CONNECTION_ERROR;
			}
			output = _constat == CONNECTION_CLOSING;
		}
		
		return output;
	}
	

	void __fastcall client::_onconnect(TObject* Sender, TCustomWinSocket *Socket){
		_constat = CONNECTION_CONNECTED;
		if(OnConnect != NULL){
			OnConnect(this);
		}
	}
	

	void __fastcall client::_ondisconnect(TObject* Sender, TCustomWinSocket *Socket){
		_constat = CONNECTION_DISCONNECTED;
		if(OnDisconnect != NULL){
			OnDisconnect(this);
		}
	}
	
	void __fastcall client::_onread(TObject* Sender, TCustomWinSocket* Socket){
		readSocket(Socket);
		if(OnRead!= NULL){
			OnRead(this);
		}
	}
	

	void __fastcall client::_onwrite(TObject* Sender, TCustomWinSocket* Socket){
		CRTLK(out_stream_lock);
		sendOut(Socket);
	}
	
	void __fastcall client::_onerror(TObject* Sender, TCustomWinSocket* Socket, TErrorEvent ev, int& ErrorCode){
		if(_constat == CONNECTION_WAITING && OnFailedConnect != NULL){
			OnFailedConnect(this);
		}
		
		//set the _constat
		if(ev == eeConnect || ev == eeDisconnect || ev == eeAccept || ev == eeLookup){
			_constat = CONNECTION_ERROR;
		}
		
		if(OnError == NULL){
			this->disconnect();
		}else{
			OnError(this, ev, ErrorCode);
		}
		
		//set the error code
		ErrorCode = 0;
	}
	
	bool client::send(const unsigned char * buffer, const unsigned int& length){
		return (_constat == CONNECTION_CONNECTED) ? client_base::send(_socket->Socket,buffer,length) : false;
	}
	
//----------------------------------server---------------------------------------//

	bool serverClientSocket::send(const unsigned char * buffer, const unsigned int& length){
		return _data.send<serverClientSocket>(this, buffer, length);
	}

	server::~server(){
		stop();
	}
	
	server::server(int port)
		:_prt(port), OnClientConnect(NULL), OnClientDisconnect(NULL)
			,OnClientError(NULL), OnClientRead(NULL), OnError(NULL)
			,OnClientCreated(NULL){
	}
	
	bool server::stop(){
		bool output = false;
		if(_socket != NULL){
			try{
				_socket->Close();
			}catch(...){}
			__try{
				delete _socket;
			}__except(EXCEPTION_EXECUTE_HANDLER){}
			_socket = NULL;
			output = true;
		}
		return output;
	}
	
	bool server::listen(){
		bool output = false;
		if(_socket == NULL){
			_socket = new TServerSocket(NULL);
			_socket->Port = _prt;
			_socket->OnClientRead = _onclientread;
			_socket->OnClientWrite = _onclientwrite;
			_socket->OnGetSocket = _ongetclientsocket;
			_socket->OnClientConnect = _onclientconnect;
			_socket->OnClientDisconnect = _onclientdisconnect;
			_socket->OnClientError = _onclienterror;
			
			try{
				_socket->Open();
				output = true;
			}catch(...){
				stop();
			}
		}
		return output;
	}
	
	bool server::listen(int port){
		if(_socket == NULL){
			_prt = port;
			return listen();
		}
		return false;
	}
	
	void __fastcall server::_ongetclientsocket(TObject * Sender, int socket, TServerClientWinSocket* &ClientSocket){
		ClientSocket = new serverClientSocket(socket, _socket->Socket);
		if(OnClientCreated != NULL){
			OnClientCreated(reinterpret_cast<serverClientSocket*>(ClientSocket));
		}
	}
	
	void __fastcall server::_onclientconnect(TObject* Sender, TCustomWinSocket *Socket){
		if(OnClientConnect != NULL){
			OnClientConnect(reinterpret_cast<serverClientSocket*>(Socket));
		}
	}
	
	void __fastcall server::_onclientdisconnect(TObject* Sender, TCustomWinSocket *Socket){
		if(OnClientDisconnect != NULL){
			OnClientDisconnect(reinterpret_cast<serverClientSocket*>(Socket));
		}
	}
	
	void __fastcall server::_onclientread(TObject* Sender, TCustomWinSocket* Socket){
		reinterpret_cast<serverClientSocket*>(Socket)->_data.readSocket(Socket);
		if(OnClientRead != NULL){
			OnClientRead(reinterpret_cast<serverClientSocket*>(Socket));
		}
	}
	
	void __fastcall server::_onclientwrite(TObject* Sender, TCustomWinSocket* Socket){
		reinterpret_cast<serverClientSocket*>(Socket)->_data.sendOut(Socket);
	}
	
	void __fastcall server::_onclienterror(TObject* Sender, TCustomWinSocket* Socket, TErrorEvent ev, int& ErrorCode){
		if(OnClientError == NULL){
			Socket->Close();
		}else{
			OnClientError(reinterpret_cast<serverClientSocket*>(Socket),ev,ErrorCode);
		}
		ErrorCode = 0;
	}
	
	void __fastcall server::_OnError(TObject * Sender, TCustomWinSocket * Socket, TErrorEvent ErrorEvent, int &ErrorCode){
		if(OnError != NULL){
			OnError(ErrorEvent, ErrorCode);
		}
		ErrorCode = 0;
	}
	
	serverClientSocket* server::getClient(int at){
		serverClientSocket* output = NULL;
		
		if(_socket == NULL || at < 0 || at >= _socket->Socket->ActiveConnections){
		}else{
			output = reinterpret_cast<serverClientSocket*>(_socket->Socket->Connections[at]);
		}
		
		return output;
	}
	
	serverClientSocket* server::getClientByIP(const AnsiString& ipAddress){
		if(_socket != NULL){
			int i = _socket->Socket->ActiveConnections;
			while(i-- > 0){
				if(_socket->Socket->Connections[i]->RemoteAddress == ipAddress){
					return reinterpret_cast<serverClientSocket*>(_socket->Socket->Connections[i]);
				}
			}
		}
		return NULL;
	}
	
	bool server::getMessageFromClient(int at, std::vector<unsigned char>& message){
		serverClientSocket* clnt = getClient(at);
		return clnt == NULL ? false : clnt->_data.getMessage(message);
	}
	
	int server::sendToAll(const unsigned char * buffer, const unsigned int& length){
		int output = -1;
		if(_socket != NULL){
			output = 0;
			int i = _socket->Socket->ActiveConnections;
			while(i-- > 0){
				if(reinterpret_cast<serverClientSocket*>(_socket->Socket->Connections[i])->send(buffer,length)){
					output++;
				}
			}
		}
		return output;
	}

}; //end namespace TCP
