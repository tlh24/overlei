
/*
 * Based on:
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


#ifndef JACK
#define JACK

#include <vector>
#include <iostream>
#include <jack/jack.h>
#include <jack/midiport.h>

#include "midi.h"

class Jack
{
  public:
    Jack();
    ~Jack();
    
    void activate();
    
    static int staticProcess(jack_nframes_t nframes, void *arg);
	 void scrub(int frame){
		 if(client){
			jack_position_t t; 
			t.frame = frame; 
			//t.valid = 0; 
			jack_transport_reposition(client, &t);
		 }
	 }
	 void play(){
		 if(client){jack_transport_start(client);}
	 }
	 void stop(){
		 if(client){jack_transport_stop(client);}
	 }
	 int getPosition(){
		 if(client){
			jack_position_t t; 
			jack_transport_query(client, &t); 
			return (int)(t.frame); 
		 } else return 0; 
	 }

	 std::list<Note> openNotes; 
	 std::list<Note> noteVector; 
  private:
    
    int previousFrame;
    
    bool record;
    
    int playbackIndex;
	 int m_recChan = 0; 
    
    std::vector<MidiEvent> eventVector;
/*store events in order (set) for playback & rapid insertion & removal.
 * thoughts: want minimal (or no) redundant data, so we don't have to worry about coherency.
 * initially I thought that keeping the note on and off information together makes sense, 
 * but this means we must do a sort() whenever we are playing --
 * which will of course be frequent!
 * However, if we store everything as pure MIDI events, 
 * time code and payload, 
 * then when moving a note around -- or deleting a note --
 * we have to find the corresponding note-off
 * which is, of course, an iteration, albiet short. 
 * E.g. while dragging notes, have to calculate all note durations, move, re-insert note off events ... 
 * seems rather unnatural, given note start and duration is a more sensible data representation.
 * However, if we store start and duration, then playback becomes difficult!
 * Yes, storing raw MIDI events makes more sense.  
 * There will be some data dependency when drawing & computation required, but it, really, will not be that bad. 
 * Probably want one set per channel, to make the search a bit easier... 
 * But then will have to sort when merging channels. 
 */
    
    int process(jack_nframes_t nframes);
    
    jack_client_t*  client;
    jack_port_t*    inputPort;
    jack_port_t*    outputPort;
};

#endif 
