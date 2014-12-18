
/*
  Program RecordPlayback: A MIDI tool
  Author: Harry van Haaren
  E-mail: harryhaaren@gmail.com
  Copyright (C) 2010 Harry van Haaren

  PrintJackMidi is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  PrintJackMidi is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with PrintJackMidi.  If not, see <http://www.gnu.org/licenses/>. */


#ifndef MIDI
#define MIDI

#include <iostream>
#include <cstring>

class MidiEvent{
public:
	long frame;
	unsigned char data[3];
	MidiEvent(){}
	MidiEvent(long inFrame, unsigned char* inData){
		frame = inFrame;
		memcpy( data, inData, 3 );
	}
};

class Note{
public:
	unsigned char m_chan; 
	unsigned char m_tone; 
	unsigned char m_vel_on; 
	unsigned char m_vel_off; 
	long m_start; 
	long m_end;
	Note(){}
	Note(unsigned char chan, unsigned char tone, 
		  unsigned char vel_on, unsigned char vel_off,
		  long start, long end){
		m_chan = chan; 
		m_tone = tone; 
		m_vel_on = vel_on; 
		m_vel_off = vel_off; 
		m_start = start; 
		m_end = end; 
	}
	void Close(unsigned char vel_off, long end){
		m_vel_off = vel_off; 
		m_end = end; 
	}
}; 
#endif 
