#include "MantaSettingsParser.h"
#include "MantaMidiSettings.h"
#include <iostream>
#include <fstream>

#define DEFINE_MASTER_CONFIG_DEFAULT(key, value)   m_configDefaults_Master[key] = value
#define DEFINE_CONFIG_DEFAULT(key, value)          m_configDefaults_Select[key] = value

// stackoverflow
const string trim(const string& pString, const string& pWhitespace)
{
    const size_t beginStr = pString.find_first_not_of(pWhitespace);
    if (beginStr == string::npos)
    {
        // no content
        return "";
    }

    const size_t endStr = pString.find_last_not_of(pWhitespace);
    const size_t range = endStr - beginStr + 1;

    return pString.substr(beginStr, range);
}

MantaSettingsParser::MantaSettingsParser(MantaMidiSettings *pSettings)
{
    m_pSettings = pSettings;

    // Master Settings
    DEFINE_MASTER_CONFIG_DEFAULT("Velocity", "1");

    // Pad Settings
    DEFINE_MASTER_CONFIG_DEFAULT("Pad_Mode", "0");
    DEFINE_MASTER_CONFIG_DEFAULT("Pad_MonoCC", "11");
    DEFINE_MASTER_CONFIG_DEFAULT("Pad_InactiveColor", "0");
    DEFINE_MASTER_CONFIG_DEFAULT("Pad_OnColor", "1");
    DEFINE_MASTER_CONFIG_DEFAULT("Pad_OffColor", "0");
    DEFINE_MASTER_CONFIG_DEFAULT("Pad_Channel", "1");

    // Master Pad Settings

    char extra[16];
    char strResult[64];
    // Individual Pad Settings
    for(int i=1; i <= 48; ++i)
    {
        sprintf(strResult, "Pad_%d", i);
        sprintf(extra, "%d 1", i + 32);
        DEFINE_CONFIG_DEFAULT(strResult, extra);
    }

    /* Pad Receive */
    /*unsigned char m_padLEDChannel[numPads];
    unsigned char m_AmberLEDMidi[numPads];
    unsigned char m_RedLEDMidi[numPads];*/

    // Button Settings
    for (int i = 1; i <= 4; ++i)
    {
        sprintf(strResult, "Button_%d_MIDI", i);
        sprintf(extra, "%d 1", 102 + i);
        DEFINE_CONFIG_DEFAULT(strResult, extra);

        sprintf(strResult, "Button_%d_Mode", i);
        sprintf(extra, "%d", 1);
        DEFINE_CONFIG_DEFAULT(strResult, extra);

        sprintf(strResult, "Button_%d_InactiveColor", i);
        sprintf(extra, "%d", 0);
        DEFINE_CONFIG_DEFAULT(strResult, extra);

        sprintf(strResult, "Button_%d_OnColor", i);
        sprintf(extra, "%d", 1);
        DEFINE_CONFIG_DEFAULT(strResult, extra);

        sprintf(strResult, "Button_%d_OffColor", i);
        sprintf(extra, "%d", 0);
        DEFINE_CONFIG_DEFAULT(strResult, extra);
    }

    // Slider Settings
    /* Sliders */
    DEFINE_CONFIG_DEFAULT("Slider_0_MIDI", "1 1");
    DEFINE_CONFIG_DEFAULT("Slider_1_MIDI", "2 1");
    DEFINE_CONFIG_DEFAULT("Slider_0_Mode", "0");
    DEFINE_CONFIG_DEFAULT("Slider_1_Mode", "0");
}

bool MantaSettingsParser::ReadCollFile(const char *fileName)
{
    cout << "Reading " << fileName << " as settings file" << endl;
    string line;
    ifstream fin;
    fin.open(fileName);

    if (fin.is_open())
    {
        int len = 0;
        int commaPos = 0;
        int colonPos = 0;

        while( fin.good() )
        {
            getline(fin, line);
            len = line.length();
            commaPos = line.find(',');
            colonPos = line.find(';');

            string testStr = line.substr(0, colonPos);

            string key = trim(testStr.substr(0, commaPos));
            string value = trim(testStr.substr(commaPos + 1, testStr.length() - commaPos - 1));

            int keyType = IsValidKey(key);
            if (keyType)
                AssignKeyToValue(keyType, key, value);
            if (key.length() > 0)
            {
                cout << "PARSE ERROR: Invalid key \"" << key << "\"" << endl;
            }
        }
    }
    else
    {
        cout << "No file of this name: aborting..." << endl;
        exit(-1);
    }

    fin.close();

    return false;
}

int MantaSettingsParser::IsValidKey(string key)
{   
    if (m_configDefaults_Master.count(key) > 0)
        return 1;
    else if (m_configDefaults_Select.count(key) > 0)
        return 2;
    else
        return 0;
}

void MantaSettingsParser::AssignKeyToValue(int type, string key, string value)
{
    if (1 == type)
        m_configDefaults_Master[key] = value;
    else if (2 == type)
        m_configDefaults_Select[key] = value;
}

void MantaSettingsParser::ParseKey(const string key, string &type, string &function, unsigned long &index)
{
    const size_t _first = key.find_first_of("_");
    const size_t _last = key.find_last_of("_");
    type = key.substr(0, _first);
    function = key.substr(_last + 1);
    index = string::npos;
    if (_first != _last)
        index = atoi(key.substr(_first + 1, (_last - _first - 1)).c_str());
    else
    {
        int iTmp = atoi(function.c_str());
        // If the atoi is zero, it didn't work, so the function is a word instead of a number
        if (0 != iTmp)
            // if it's a number, it's an index.
            index = iTmp;
    }
}

void MantaSettingsParser::ParseMidiValue(const string &value, int &midiNote, int &midiChan)
{
    const char *strValue = value.c_str();
    char * pch;
    pch = strtok((char *)strValue, " ");

    if (pch != NULL)
    {
        midiNote = atoi(pch);

        pch = strtok (NULL, " " );
        if (pch != NULL)
            midiChan = atoi(pch);
        else
            midiChan = 1;
    }
}

bool MantaSettingsParser::UpdateSettings()
{
    bool bRet = true;

    for(map<string, string>::iterator itr = m_configDefaults_Master.begin(); itr != m_configDefaults_Master.end(); ++itr)
    {
        string key = (*itr).first;
        string val = (*itr).second;

        if (1 == IsValidKey(key))
            bRet &= UpdateMasterSetting(key, val);
    }

    for(map<string, string>::iterator itr = m_configDefaults_Select.begin(); itr != m_configDefaults_Select.end(); ++itr)
    {
        string key = (*itr).first;
        string val = (*itr).second;

        if (2 == IsValidKey(key))
            bRet &= UpdateSelectSetting(key, val);
    }

    return bRet;
}

bool MantaSettingsParser::UpdateMasterSetting(const string& key, const string& val)
{
    bool bRet = false;
    bool debug = m_pSettings->GetDebugMode();

    int iVal = atoi(val.c_str());

    string type, function;
    unsigned long index;

    ParseKey(key, type, function, index);

    if (type == "Pad")
    {
        if (function == "Mode")
            m_pSettings->SetPad_Mode((PadValMode)iVal);
        else if (function == "LayoutTitle")
            m_pSettings->SetPadLayoutTitle(val.c_str());
        else if (function == "MonoCC")
            m_pSettings->SetPad_MonoCCNumber(iVal);
        else if (function == "InactiveColor")
        {
            m_pSettings->SetAllPadInactiveColor((Manta::LEDState)iVal);
            if (debug) cout << " Pad Inactive Color: " << iVal << endl;
        }
        else if (function == "OnColor")
        {
            m_pSettings->SetAllPadOnColor((Manta::LEDState)iVal);
            if (debug) cout << " Pad On Color: " << iVal << endl;
        }
        else if (function == "OffColor")
        {
            m_pSettings->SetAllPadOffColor((Manta::LEDState)iVal);
            if (debug) cout << " Pad Off Color: " << iVal << endl;
        }
        else if (function == "Channel")
            m_pSettings->SetPad_Channel(iVal);
        else
        {
            int midi = -1;
            int chan = 1;
            ParseMidiValue(val, midi, chan);
            if (debug) cout << " Pad " << index << " Midi: " << midi << " " << chan << endl;
            m_pSettings->SetPad(index - 1, chan - 1, midi - 1);
        }
    }
    else if (type == "Button")
    {
        if (function == "MIDI")
        {
            int midi = -1;
            int chan = 1;
            ParseMidiValue(val, midi, chan);

            if (index == 1 || index == string::npos)
            {
                m_pSettings->SetButton_Midi(0, midi);
                m_pSettings->SetButton_Channel(0, chan);
            }
            else if (index == 2 || index == string::npos)
            {
                m_pSettings->SetButton_Midi(1, midi);
                m_pSettings->SetButton_Channel(0, chan);
            }
            else if (index == 3 || index == string::npos)
            {
                m_pSettings->SetButton_Midi(2, midi);
                m_pSettings->SetButton_Channel(0, chan);
            }
            else if (index == 4 || index == string::npos)
            {
                m_pSettings->SetButton_Midi(3, midi);
                m_pSettings->SetButton_Channel(0, chan);
            }
        }
        else if (function == "Mode")
        {
            if (index == 1 || index == string::npos)
                m_pSettings->SetButton_Mode(0, (ButtonMode)iVal);
            else if (index == 2 || index == string::npos)
                m_pSettings->SetButton_Mode(1, (ButtonMode)iVal);
            else if (index == 3 || index == string::npos)
                m_pSettings->SetButton_Mode(2, (ButtonMode)iVal);
            else if (index == 4 || index == string::npos)
                m_pSettings->SetButton_Mode(3, (ButtonMode)iVal);
        }
        else if (function == "InactiveColor")
        {
            if (index == 1 || index == string::npos)
                m_pSettings->SetButton_InactiveColor(0, (Manta::LEDState)iVal);
            else if (index == 2 || index == string::npos)
                m_pSettings->SetButton_InactiveColor(1, (Manta::LEDState)iVal);
            else if (index == 3 || index == string::npos)
                m_pSettings->SetButton_InactiveColor(2, (Manta::LEDState)iVal);
            else if (index == 4 || index == string::npos)
                m_pSettings->SetButton_InactiveColor(3, (Manta::LEDState)iVal);
        }
        else if (function == "OnColor")
        {
            if (index == 1 || index == string::npos)
                m_pSettings->SetButton_OnColor(0, (Manta::LEDState)iVal);
            else if (index == 2 || index == string::npos)
                m_pSettings->SetButton_OnColor(1, (Manta::LEDState)iVal);
            else if (index == 3 || index == string::npos)
                m_pSettings->SetButton_OnColor(2, (Manta::LEDState)iVal);
            else if (index == 4 || index == string::npos)
                m_pSettings->SetButton_OnColor(3, (Manta::LEDState)iVal);
        }
        else if (function == "OffColor")
        {
            if (index == 1 || index == string::npos)
                m_pSettings->SetButton_OffColor(0, (Manta::LEDState)iVal);
            else if (index == 2 || index == string::npos)
                m_pSettings->SetButton_OffColor(1, (Manta::LEDState)iVal);
            else if (index == 3 || index == string::npos)
                m_pSettings->SetButton_OffColor(2, (Manta::LEDState)iVal);
            else if (index == 4 || index == string::npos)
                m_pSettings->SetButton_OffColor(3, (Manta::LEDState)iVal);
        }
    }
    else if (type == "Slider")
    {
        if (function == "MIDI")
        {
            int midi = -1;
            int chan = 1;
            ParseMidiValue(val, midi, chan);

            if (index == 0 || index == string::npos)
            {
                m_pSettings->SetSlider_Midi(0, midi);
                m_pSettings->SetSlider_Channel(0, chan);
            }
            else if (index == 1 || index == string::npos)
            {
                m_pSettings->SetSlider_Midi(1, midi);
                m_pSettings->SetSlider_Channel(1, chan);
            }
        }
        else if (function == "Mode")
        {
            if (index == 0 || index == string::npos)
                m_pSettings->SetSlider_Mode(0, (SliderMode)iVal);
            else if (index == 1 || index == string::npos)
                m_pSettings->SetSlider_Mode(1, (SliderMode)iVal);
        }
    }
    else
    {
        if (key == "Velocity")
            m_pSettings->SetUseVelocity(iVal);
    }

    return bRet;
}

bool MantaSettingsParser::UpdateSelectSetting(const string& key, const string& val)
{
    bool bRet = false;
    bool debug = m_pSettings->GetDebugMode();

    int iVal = atoi(val.c_str());
    if (IsValidKey(key))
    {
        string type, function;
        unsigned long index;

        ParseKey(key, type, function, index);

        if (type == "Pad")
        {
            if (function == "Mode")
                m_pSettings->SetPad_Mode((PadValMode)iVal);
            else if (function == "LayoutTitle")
                m_pSettings->SetPadLayoutTitle(val.c_str());
            else if (function == "MonoCC")
                m_pSettings->SetPad_MonoCCNumber(iVal);
            else if (function == "InactiveColor")
            {
                m_pSettings->SetAllPadInactiveColor((Manta::LEDState)iVal);
                if (debug) cout << " Pad Inactive Color: " << iVal << endl;
            }
            else if (function == "OnColor")
            {
                m_pSettings->SetAllPadOnColor((Manta::LEDState)iVal);
                if (debug) cout << " Pad On Color: " << iVal << endl;
            }
            else if (function == "OffColor")
            {
                m_pSettings->SetAllPadOffColor((Manta::LEDState)iVal);
                if (debug) cout << " Pad Off Color: " << iVal << endl;
            }
            else if (function == "Channel")
                m_pSettings->SetPad_Channel(iVal);
            else
            {
                int midi = -1;
                int chan = 1;
                ParseMidiValue(val, midi, chan);
                if (debug) cout << " Pad " << index << " Midi: " << midi << " " << chan << endl;
                m_pSettings->SetPad(index - 1, chan - 1, midi - 1);
            }
        }
        else if (type == "Button")
        {
            if (function == "MIDI")
            {
                int midi = -1;
                int chan = 1;
                ParseMidiValue(val, midi, chan);

                if (index == 1 || index == string::npos)
                {
                    m_pSettings->SetButton_Midi(0, midi);
                    m_pSettings->SetButton_Channel(0, chan);
                }
                else if (index == 2 || index == string::npos)
                {
                    m_pSettings->SetButton_Midi(1, midi);
                    m_pSettings->SetButton_Channel(0, chan);
                }
                else if (index == 3 || index == string::npos)
                {
                    m_pSettings->SetButton_Midi(2, midi);
                    m_pSettings->SetButton_Channel(0, chan);
                }
                else if (index == 4 || index == string::npos)
                {
                    m_pSettings->SetButton_Midi(3, midi);
                    m_pSettings->SetButton_Channel(0, chan);
                }
            }
            else if (function == "Mode")
            {
                if (index == 1 || index == string::npos)
                    m_pSettings->SetButton_Mode(0, (ButtonMode)iVal);
                else if (index == 2 || index == string::npos)
                    m_pSettings->SetButton_Mode(1, (ButtonMode)iVal);
                else if (index == 3 || index == string::npos)
                    m_pSettings->SetButton_Mode(2, (ButtonMode)iVal);
                else if (index == 4 || index == string::npos)
                    m_pSettings->SetButton_Mode(3, (ButtonMode)iVal);
            }
            else if (function == "InactiveColor")
            {
                if (index == 1 || index == string::npos)
                    m_pSettings->SetButton_InactiveColor(0, (Manta::LEDState)iVal);
                else if (index == 2 || index == string::npos)
                    m_pSettings->SetButton_InactiveColor(1, (Manta::LEDState)iVal);
                else if (index == 3 || index == string::npos)
                    m_pSettings->SetButton_InactiveColor(2, (Manta::LEDState)iVal);
                else if (index == 4 || index == string::npos)
                    m_pSettings->SetButton_InactiveColor(3, (Manta::LEDState)iVal);
            }
            else if (function == "OnColor")
            {
                if (index == 1 || index == string::npos)
                    m_pSettings->SetButton_OnColor(0, (Manta::LEDState)iVal);
                else if (index == 2 || index == string::npos)
                    m_pSettings->SetButton_OnColor(1, (Manta::LEDState)iVal);
                else if (index == 3 || index == string::npos)
                    m_pSettings->SetButton_OnColor(2, (Manta::LEDState)iVal);
                else if (index == 4 || index == string::npos)
                    m_pSettings->SetButton_OnColor(3, (Manta::LEDState)iVal);
            }
            else if (function == "OffColor")
            {
                if (index == 1 || index == string::npos)
                    m_pSettings->SetButton_OffColor(0, (Manta::LEDState)iVal);
                else if (index == 2 || index == string::npos)
                    m_pSettings->SetButton_OffColor(1, (Manta::LEDState)iVal);
                else if (index == 3 || index == string::npos)
                    m_pSettings->SetButton_OffColor(2, (Manta::LEDState)iVal);
                else if (index == 4 || index == string::npos)
                    m_pSettings->SetButton_OffColor(3, (Manta::LEDState)iVal);
            }
        }
        else if (type == "Slider")
        {
            if (function == "MIDI")
            {
                int midi = -1;
                int chan = 1;
                ParseMidiValue(val, midi, chan);

                if (index == 0 || index == string::npos)
                {
                    m_pSettings->SetSlider_Midi(0, midi);
                    m_pSettings->SetSlider_Channel(0, chan);
                }
                else if (index == 1 || index == string::npos)
                {
                    m_pSettings->SetSlider_Midi(1, midi);
                    m_pSettings->SetSlider_Channel(1, chan);
                }
            }
            else if (function == "Mode")
            {
                if (index == 0 || index == string::npos)
                    m_pSettings->SetSlider_Mode(0, (SliderMode)iVal);
                else if (index == 1 || index == string::npos)
                    m_pSettings->SetSlider_Mode(1, (SliderMode)iVal);
            }
        }
        else
        {
            if (key == "Velocity")
                m_pSettings->SetUseVelocity(iVal);
        }
    }

    return bRet;
}

void MantaSettingsParser::PrintSettings()
{
    for(map<string, string>::iterator itr = m_configDefaults_Master.begin(); itr != m_configDefaults_Master.end(); ++itr)
        cout << (*itr).first << ", " << (*itr).second << ";" << endl;

    for(map<string, string>::iterator itr = m_configDefaults_Select.begin(); itr != m_configDefaults_Select.end(); ++itr)
        cout << (*itr).first << ", " << (*itr).second << ";" << endl;
}
