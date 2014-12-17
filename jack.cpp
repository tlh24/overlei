
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

#include <list>
#include <set>
#include "jack.h"

using namespace std;

Jack::Jack(){
  record = true;
  
  previousFrame = 0;
  playbackIndex = 0;
  m_recChan = 0; 
  
  std::cout << "Jack()" << std::flush;
  
  if ((client = jack_client_open("MidiRecPlay", JackNullOption, NULL)) == 0){
    std::cout << "jack server not running?" << std::endl;
  }
  
  inputPort  = jack_port_register (client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  outputPort = jack_port_register (client, "midi_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
  
  const char** ports = jack_get_ports(client, "", "", 0);
  int i = 0; 
  std::cout << "List of ports:" << std::endl; 
  while(ports[i]){
	std::cout << ports[i] << std::endl; 
	i++; 
  }
  free(ports); 
  
  jack_set_process_callback (client, staticProcess, static_cast<void*>(this));
  
  std::cout << "\t\t\tDone!" << std::endl;
 
  // for testing playback: one note on every 22050 frames (2 per second at 44.1 kHz samplerate)
  unsigned char array[] = {144, 48, 32};
  
  for(int i = 0; i < 2; i++){
    eventVector.push_back( MidiEvent( i * 22050, array ) );
  }
  
}

Jack::~Jack(){
  std::cout << "~Jack()" << std::endl;
}

void Jack::activate(){
  std::cout << "activate()" << std::flush;
  
  if (jack_activate(client) != 0){
    std::cout<<  "cannot activate client" << std::endl;
    return;
  }
  std::cout << "\t\tDone!" << std::endl;
  if(jack_connect(client, 
			"a2j:UMX 490 [20] (capture): UMX 490 MIDI 1", 
			"MidiRecPlay:midi_in"))
	  std::cout << "could not connect input port." << std::endl; 
  if(jack_connect(client, 
			"MidiRecPlay:midi_out",
			"a2j:UMX 490 [20] (playback): UMX 490 MIDI 1"
			))
	  std::cout << "could not connect output port." << std::endl; 
  
}

int Jack::staticProcess(jack_nframes_t nframes, void *arg)
{
  return static_cast<Jack*>(arg)->process(nframes);
}

int Jack::process(jack_nframes_t nframes)
{
  jack_midi_event_t in_event;
  jack_nframes_t event_index = 0;
  jack_position_t         position;
  jack_transport_state_t  transport;
  
  // get the port data
  void* inputPortBuf = jack_port_get_buffer( inputPort, nframes );
  void* outputPortBuf = jack_port_get_buffer( outputPort, nframes );
  
  // ensure to clear the buffer!! Otherwise the same event will be sent every nframes!
  jack_midi_clear_buffer(outputPortBuf);
  
  // get the transport state of the JACK server
  transport = jack_transport_query( client, &position );
  
  // input: get number of events, and process them.
  jack_nframes_t event_count = 
	jack_midi_get_event_count( inputPortBuf );
  if(event_count > 0){
    for(int i=0; i<event_count; i++){
      jack_midi_event_get(&in_event, inputPortBuf, i);
      
      // Using "cout" in the JACK process() callback is NOT realtime, this is
      // used here for simplicity.
      std::cout << "Frame " << position.frame << "  Event: " << i << " SubFrame#: " << in_event.time << " \tMessage:\t"
                << (long)in_event.buffer[0] << "\t" << (long)in_event.buffer[1]
                << "\t" << (long)in_event.buffer[2] << std::endl;
					 
      long time = position.frame + (long)in_event.time; 
      eventVector.push_back( MidiEvent( time, (unsigned char*)in_event.buffer ) );
		unsigned char ev = in_event.buffer[0]; 
		unsigned char tone = in_event.buffer[1]; 
		unsigned char vel = in_event.buffer[2];
		//recording accepts from all channels... 
		if(ev >= 0x90 && ev <= 0x9f){
			openNotes.push_back( Note(m_recChan, tone, vel, 0,
					time, 0) ); 
			printf("opened an note, %d\n", tone); 
		}
		if(ev >= 0x80 && ev <= 0x8f){
			std::list<Note>::iterator it;
			for(it=openNotes.begin(); it != openNotes.end(); ++it){
				// you have to release every key that you press.
				if((*it).m_chan == m_recChan && (*it).m_tone == tone){
					(*it).Close(vel, time); 
					noteVector.push_back((*it)); 
					it = openNotes.erase(it); 
					printf("closed a note, %d %d\n", tone, time); 
				}
			}
		}
      //cout << "event.back() frame = " << eventVector.back().frame << endl;
    }
  }
  
  for( int i = 0; i < nframes; i++ ){
    if ( playbackIndex < eventVector.size() &&
         position.frame > eventVector.at(playbackIndex).frame ){
      // print the MIDI event that's getting played back (NON-RT!!)
      cout << "Playback event! Frame = " << eventVector.at(playbackIndex).frame << "  Data  "
           << (long)eventVector.at(playbackIndex).data[0] << "\t" << (long)eventVector.at(playbackIndex).data[1]
           << "\t" << (long)eventVector.at(playbackIndex).data[2] << endl;
      
      
      // here we write the MIDI event into the output port's buffer
      unsigned char* buffer = jack_midi_event_reserve( outputPortBuf, 0, 3);
      
      if( buffer == 0 ){
        std::cout << "Error: write MIDI failed! write buffer == 0" << std::endl;
      } else {
        //cout << "JC::writeMidi() " << b1 << ", " << b2 << ", " << b3 << endl; 
        buffer[0] = eventVector.at(playbackIndex).data[0];
        buffer[1] = eventVector.at(playbackIndex).data[1];
        buffer[2] = eventVector.at(playbackIndex).data[2];
      }
      playbackIndex++;
    }
  }
  
  // reset to start of eventVector when we "rewind" jack's transport
  if ( position.frame == 0 ){
    playbackIndex = 0;
  }
  
  
  return 0;
}
