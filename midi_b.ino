//-----------------------------------------------------------------------------
// Based on: https://beammyselfintothefuture.wordpress.com/2015/01/28/sensing-hit-velocity-and-quick-subsequent-hits-of-a-piezo-with-an-arduino-teensy/
//-----------------------------------------------------------------------------

#define DEFAULT_PAD_THRESHOLD   30
#define NUMBER_OF_PADS          19
#define IGNORE_TIME             47      // ms
#define MIDI_B_CHANNEL          10      // VST instrument channel
#define MAX_MIDI_VELOCITY       127
#define MIN_MIDI_VELOCITY       80
#define HIT_DURATION            3       // ms

//-----------------------------------------------------------------------------
// GM mapping
//-----------------------------------------------------------------------------
#define BASS_DRUM               36
#define ACOUSTIC_SNARE_DRUM     38
#define ELECTRIC_SNARE_DRUM     40
#define SIDE_STICK              37
#define LOW_TOM                 45
#define LOW_FLOOR_TOM           41
#define HIGH_FLOOR_TOM          43
#define LOW_MID_TOM             47
#define HI_MID_TOM              48
#define PEDAL_HI_HAT            44
#define OPEN_HI_HAT             46
#define CLOSED_HI_HAT           42
#define CRASH_CYMBAL_1          49
#define CRASH_CYMBAL_2          57
#define SPLASH_CYMBAL           55
#define TAMBOURINE              54
#define CUSTOM_1                62
#define CUSTOM_2                60
#define CUSTOM_3                83

//-----------------------------------------------------------------------------
// Pad class
//-----------------------------------------------------------------------------
class Pad
{
public:
    //-----------------------------------------------------------------------------
    // constructor
    //-----------------------------------------------------------------------------
    Pad(uint8_t pad_id, uint8_t pad_note)
        : _pad_id {pad_id},
		  _default_pad_threshold {DEFAULT_PAD_THRESHOLD},
		  _pad_note {pad_note},
          _retrigger_indicator {false},
		  _highest_raw_value {0}
    {}

    //-----------------------------------------------------------------------------
    // calibrate_velocity
    //-----------------------------------------------------------------------------
    uint8_t calibrate_velocity(uint8_t velocity)
    {
        uint8_t new_velocity;

        switch (_pad_note)
        {
            case BASS_DRUM:
                new_velocity = map(velocity, 80, 127, 105, 127);
                break;
            default:
                new_velocity = map(velocity, 80, 127, 80, 150);
                new_velocity = (new_velocity > 128) ? 127 : new_velocity;
                break;
        }

        return new_velocity;
    }

    //-----------------------------------------------------------------------------
    // get_raw_hit_value
    //-----------------------------------------------------------------------------
    void get_raw_hit_value(uint16_t raw_analog_value)
    {
        _hit_start_time = millis();
        while ((_hit_start_time + HIT_DURATION) > millis())
        {
            if (raw_analog_value > _highest_raw_value)
            {
                _highest_raw_value = raw_analog_value;
                raw_analog_value = analogRead(_pad_id);
            }
        }
    }

    //-----------------------------------------------------------------------------
    // check_pad
    //-----------------------------------------------------------------------------
    void check_pad()
    {
        if (_retrigger_indicator == false)
        {
            uint16_t raw_analog_value = analogRead(_pad_id);
            if (raw_analog_value > _default_pad_threshold)
            {
                get_raw_hit_value(raw_analog_value);

                uint8_t velocity = map(_highest_raw_value, 0, 1023, MIN_MIDI_VELOCITY, MAX_MIDI_VELOCITY);
                velocity = calibrate_velocity(velocity);

                usbMIDI.sendNoteOn(_pad_note, velocity, MIDI_B_CHANNEL);
                usbMIDI.sendNoteOff(_pad_note, velocity, MIDI_B_CHANNEL);

                _retrigger_indicator = true;
                _highest_raw_value = 0;
                _hit_ignore_time = millis();
            }
        }
        else
        {
            // avoid fake triggers
            if ((_hit_ignore_time + IGNORE_TIME) < millis())
            {
                _retrigger_indicator = false;
			}
        }
    }

private:
    uint8_t _pad_id;
    uint8_t _pad_note;
    uint8_t _default_pad_threshold;
    bool _retrigger_indicator;
    uint16_t _highest_raw_value;
    uint32_t _hit_start_time;
    uint32_t _hit_ignore_time;
};

const uint8_t pad_notes[NUMBER_OF_PADS] =
    {
        BASS_DRUM, ACOUSTIC_SNARE_DRUM,
        ELECTRIC_SNARE_DRUM, SIDE_STICK, LOW_FLOOR_TOM, HIGH_FLOOR_TOM, LOW_TOM,
        LOW_MID_TOM, HI_MID_TOM, PEDAL_HI_HAT, OPEN_HI_HAT, CLOSED_HI_HAT,
        CRASH_CYMBAL_1, CRASH_CYMBAL_2, TAMBOURINE, CUSTOM_1, SPLASH_CYMBAL,
        CUSTOM_2, CUSTOM_3
    };

// pin numbers
const uint8_t pad_ids[NUMBER_OF_PADS] =
    {
        31, 22, 29, 16, 14, 17, A14, 19, 15,
        30, 27, 21, 20, 10, 28, 11, 23, 18, 26
    };


#include <vector>
std::vector<Pad> pad_vector;

//-----------------------------------------------------------------------------
// setup function
//-----------------------------------------------------------------------------
void setup()
{
    for (uint8_t i = 0; i < NUMBER_OF_PADS; ++i)
    {
        pad_vector.push_back(Pad(pad_ids[i], pad_notes[i]));
    }
}

//-----------------------------------------------------------------------------
// loop function
//-----------------------------------------------------------------------------
void loop()
{
    for (Pad &pad : pad_vector)
        pad.check_pad();
}
