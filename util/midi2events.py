# midi to log file utility, generated with Claude.

import mido
import argparse
from pathlib import Path

def midi_to_log(midi_file, output_file, use_ms=True):
    """
    Extract note-on events from a MIDI file and log them with timing information.
    
    Args:
        midi_file (str): Path to the MIDI file
        output_file (str): Path to the output log file
        use_ms (bool): If True, time is in milliseconds; otherwise, in beats
    """
    # Load the MIDI file
    mid = mido.MidiFile(midi_file)
    
    # Get the track (assuming there's only one track)
    if len(mid.tracks) != 1:
        print(f"Warning: Expected 1 track, but found {len(mid.tracks)}. Using the first track.")
    
    track = mid.tracks[0]
    
    # Calculate tempo (in microseconds per beat, defaults to 500,000 if not specified)
    tempo = 500000  # Default tempo
    first_event_time = -1 
    # Open output file for writing
    with open(output_file, 'w') as f:
        f.write("Time\tNote\tVelocity\n")
        
        current_time = 0  # Time in ticks
        
        for msg in track:
            # Update current time
            current_time += msg.time
            if (first_event_time < 0):
                first_event_time = msg.time
            note_relative_time = current_time - first_event_time

            # Update tempo if found
            if msg.type == 'set_tempo':
                tempo = msg.tempo
            
            # Log note-on events (ignore note-off which have velocity 0)
            if msg.type == 'note_on' and msg.velocity > 0:
                if use_ms:
                    # Convert ticks to milliseconds
                    # time in ms = (time in ticks) * (tempo in μs/beat) / (ticks per beat) / 1000
                    time_ms = note_relative_time * tempo / mid.ticks_per_beat / 1000
                    f.write(f"{time_ms:.2f}\t{msg.note}\t{msg.velocity}\n")
                else:
                    # Time in beats
                    time_beats = note_relative_time / mid.ticks_per_beat
                    f.write(f"{time_beats:.4f}\t{msg.note}\t{msg.velocity}\n")

def main():
    parser = argparse.ArgumentParser(description='Extract note-on events from a MIDI file.')
    parser.add_argument('midi_file', help='Path to the MIDI file')
    parser.add_argument('--output', '-o', default='midi_notes.log', help='Output log file')
    parser.add_argument('--beats', '-b', action='store_true', help='Use beats for timing instead of milliseconds')
    
    args = parser.parse_args()
    
    midi_to_log(args.midi_file, args.output, not args.beats)
    print(f"Note-on events logged to {args.output}")

if __name__ == '__main__':
    main()
