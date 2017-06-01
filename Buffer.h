#ifndef _BUFFER_H
#define _BUFFER_H

#include <vector>
#include <algorithm>

template<typename InputIterator, typename OutputIterator>
inline void MEM_COPY(OutputIterator dest, InputIterator source, const unsigned int& length){
    std::copy(source,source+length,dest);
}

template<typename InputIterator, typename OutputIterator>
inline void MEM_COPY_BACK(OutputIterator dest, InputIterator source, const unsigned int& length){
    std::copy_backward(source,source+length,dest+length);
}

template<typename InputIterator, typename OutputIterator>
void MEM_MOVE(OutputIterator dest, InputIterator source, const unsigned int& length){
    if(dest >= source && dest < source+length){
        MEM_COPY_BACK(dest,source,length);
    }else{
        MEM_COPY(dest,source,length);
    }
}

//Buffer to act like an iostream with the random data access and searching.
template<typename T>
struct StreamBuffer{
public:
    typedef std::vector<T> buffer_type;
    typedef buffer_type::iterator iterator;
    typedef buffer_type::const_iterator const_iterator;

protected:
    buffer_type _buff;
    unsigned int _end;
    
public:
    
    StreamBuffer(const unsigned int& initialSize = 0):_end(0),_buff(initialSize){}
	StreamBuffer(const StreamBuffer& other):_end(other._end),_buff(other._buff){}

    unsigned int length() const{  return _end;    }
    virtual bool empty() const{ return _end==0; }
    unsigned int capacity() const{  return _buff.size();    }

    iterator begin(){ return _buff.begin();  }
    iterator end(){   return _buff.end();    }
    const_iterator begin() const{ return _buff.begin();   }
    const_iterator end() const{   return _buff.end(); }

    virtual void write(const T* buffer, const unsigned int& length){
        unsigned int required = _end+length;
        if(required > capacity()){
            _buff.resize(required);
        }

        MEM_COPY(begin()+_end,buffer,length);
        _end  = required;
    }
	virtual void write(const T& value){
		write(&value,1);
	}
	virtual void write(const StreamBuffer& other){
		write(other.begin(), other.length());
	}

    virtual unsigned int copy(T* buffer, unsigned int length) const{
        if(length > _end) length = _end;
        MEM_COPY(buffer,begin(),length);
        return length;
    }

    //Removes elements from the front of the buffer, without resizing
    virtual unsigned int erase(unsigned int length){
        if(length > _end){ 
			length = _end;
			clear();
		}else{
			_end -= length;
			MEM_MOVE(begin(),begin()+length,_end);
		}
        return length;
    }
	
	//erase until iterator
	virtual unsigned int erase_until(iterator ittr){
		unsigned int output = 0;
		if(ittr < begin()){
		}else if(ittr >= end()){
			output = _end;
			clear();
		}else{
			output = ittr-begin();
			output = erase(output);
		}
		return output;
	}
	virtual unsigned int erase_until(const_iterator ittr){
		return erase_until(const_cast<iterator>(ittr));
	}

    virtual unsigned int read(T* buffer, unsigned int length){
        return erase(copy(buffer,length));
    }
	
	//read until the itterator
	//ensure that the buffer has enough room
	virtual unsigned int read_until(T* buffer, iterator ittr){
		unsigned int output = 0;
		if(ittr < begin()){
		}else if(ittr >= end()){
			output = read(buffer, _end);
		}else{
			output = read(buffer, ittr-begin());
		}
		return output;
	}
	virtual unsigned int read_until(T* buffer, const_iterator ittr){
		return read_until(buffer, const_cast<iterator>(ittr));
	}

    virtual void clear(){
        _end = 0;
    }

	iterator find(T* needle, const unsigned int& needleLength){
        return find(needle, needleLength, begin());
    }
    const_iterator find(T* needle, const unsigned int& needleLength) const{
        return find(needle, needleLength, begin());
    }
	iterator find(T* needle, const unsigned int& needleLength, iterator start){
        return std::search(start,end(),needle,needle+needleLength);
    }
    const_iterator find(T* needle, const unsigned int& needleLength, const_iterator start) const{
        return std::search(start,end(),needle,needle+needleLength);
    }
    iterator find(T* needle, const unsigned int& needleLength, const unsigned int& pos){
        return find(needle,needleLength,begin()+pos);
    }
    const_iterator find(T* needle, const unsigned int& needleLength, const unsigned int& pos) const{
        return find(needle,needleLength,begin()+pos);
    }
	
	iterator find(const T& needle){
		return std::find(begin(),end(),needle);
	}
	const_iterator find(const T& needle) const{
		return std::find(begin(),end(),needle);
	}
	iterator find(const T& needle, iterator start){
		return std::find(start,end(),needle);
	}
	const_iterator find(const T& needle, const_iterator start) const{
		return std::find(start,end(),needle);
	}
	iterator find(const T& needle, const unsigned int& pos){
		return find(needle, begin()+pos);
	}
	const_iterator find(const T& needle, const unsigned int& pos) const{
		return find(needle, begin()+pos);
	}
	
};

#endif //_BUFFER_H
