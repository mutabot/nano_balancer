#pragma once
#include <ostream>
#include <chrono>
#include <ctime>
#include <iomanip>

class time_stamp_stream : public std::streambuf
{
	std::streambuf* myDest;
	std::ostream* myOwner;
	bool myIsAtStartOfLine;
protected:
	int overflow(int ch)
	{
		//  To allow truly empty lines, otherwise drop the
		//  first condition...
		if (ch != '\n' && myIsAtStartOfLine) {
			std::ostringstream buffer;
			auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());		
			buffer << std::put_time(std::localtime(&t), "%F %T") << " " << std::setfill('0') << std::setw(5) << boost::this_thread::get_id() << " ";
			myDest->sputn(buffer.str().c_str(), buffer.str().length());
		}
		myIsAtStartOfLine = ch == '\n';
		ch = myDest->sputc(ch);
		return ch;
	}

public:
	
	time_stamp_stream(std::ostream& owner)
		: myDest(owner.rdbuf())
		, myOwner(&owner)
		, myIsAtStartOfLine(false)
	{
		myOwner->rdbuf(this);
	}
	~time_stamp_stream()
	{
		if (myOwner != nullptr) {
			myOwner->rdbuf(myDest);
		}
	}
};
