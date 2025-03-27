#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Devices.Midi.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/base.h>

#include <conio.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace winrt;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Devices::Midi;
using namespace Windows::Storage::Streams;

//--------------------------------------------------------------------------------------------------------------------------

int getIndexFromString(const std::wstring& str) 
{
    bool all_digits = std::all_of(str.begin(), str.end(), [](wchar_t c) { return std::isdigit(c); });
    return (all_digits&&str.size()) ? std::stoi(str) : 0;
}

//--------------------------------------------------------------------------------------------------------------------------

int getDeviceIndexFromString(DeviceInformationCollection& devices, const std::wstring& substring )
{
    uint32_t deviceCount = devices.Size();
    for (uint32_t i = 0; i < deviceCount; ++i) 
    {
        DeviceInformation device = devices.GetAt(i);
        if (wcsstr(device.Name().c_str(), substring.c_str())!=0) 
            return i;
    }

    return -1;
}

//--------------------------------------------------------------------------------------------------------------------------

bool loadArgsFile ( const std::wstring& argsFileName, std::wstring& inputName, std::wstring& outputName ) 
{    
    std::wifstream file(argsFileName);
    if (!file.is_open()) 
    {
        std::wcerr << L"ERROR: Unable to open file: " << argsFileName << std::endl;
        return false;
    }

    std::getline(file, inputName );
    std::getline(file, outputName);
    file.close();

    if (inputName.empty() || outputName.empty()) {
        return false;
    }

    return true;}

//--------------------------------------------------------------------------------------------------------------------------

void enum_devices( bool inputs, DeviceInformationCollection& devices ) 
{
    std::wcout << L"\n";
    std::wcout << L"-----Midi "<<(inputs?"input":"outputs")<<" devices-----\n";

    Windows::Foundation::IAsyncOperation<DeviceInformationCollection> devicesOp = DeviceInformation::FindAllAsync(inputs?MidiInPort::GetDeviceSelector():MidiOutPort::GetDeviceSelector());
    devices = devicesOp.get();
    uint32_t deviceCount = devices.Size();
    if (deviceCount == 0) 
    {
        std::wcout << L"    No MIDI devices found.\n";
    } 
    else 
    {
        for (uint32_t i = 0; i < deviceCount; ++i) 
        {
            DeviceInformation device = devices.GetAt(i);
            std::wcout << L"    [" << i << L"] '" << device.Name().c_str() << L"'" << std::endl;
        }
    }
}

//--------------------------------------------------------------------------------------------------------------------------

int main(int argc, wchar_t* argv[])
{
    init_apartment();

    // Enum MIDI Devices
    DeviceInformationCollection inputs = nullptr;
    enum_devices( true, inputs  );
    DeviceInformationCollection outputs = nullptr;
    enum_devices( false, outputs );
    std::wcout << L"\n";

    // Get params
    std::wstring argsFile  = L"";
    std::wstring inputName  = L"";
    std::wstring outputName = L"";
    bool args_error = false;
    switch(argc)
    {
        case 1:
            argsFile = L"EasyMidiRouter.args";
            break;
        case 2:
            if (wcsstr(argv[1], L".args")==0)
                inputName  = argv[1];
            else
                argsFile = argv[1];
            break;
        case 3:
            inputName  = argv[1];
            outputName = argv[2];
            break;

        default:
            std::wcerr << L"ERROR: Too many params.\n";
            args_error=true;
    }

    // Read args file
    if (!argsFile.empty()) 
    {
        std::wcout << L"INFO: loading:"<<argsFile<<".\n";
        if ( loadArgsFile ( argsFile, inputName, outputName ) )
            std::wcout << L"INFO: Got Args from file. Input='"<<inputName<<L"' Output='"<<outputName<<L"'.\n";
        else
        {
            std::wcout << L"INFO: Unable to get args from file.'.\n";
            args_error=true;
        }
    }

    // Indexes to names
    if (!args_error)
    {
        int inputIndex = getIndexFromString(inputName);
        if (inputIndex>=0)
        {
            if (inputIndex<int(inputs.Size()))
                inputName=inputs.GetAt(inputIndex).Name();
            else
            {
                std::wcerr << L"ERROR: Input index bigger than input devices number.\n";
                args_error=true;
            }
        }

        int outputIndex = getIndexFromString(outputName);
        if (outputIndex>=0)
        {
            if (outputIndex<int(outputs.Size()))
                outputName=outputs.GetAt(outputIndex).Name();
            else
            {
                std::wcerr << L"ERROR: Output index bigger than output devices number.\n";
                args_error=true;
            }
        }
    }

    // Names to devices
    DeviceInformation input  = nullptr;
    DeviceInformation output = nullptr;
    if (!args_error)
    {
        int inputIndex  = getDeviceIndexFromString(inputs, inputName);
        if (inputIndex>=0)
        {
            std::wcout << L"INFO: Input found. ["<<inputIndex<<"] '"<<inputName<<L"'.\n";
            input=inputs.GetAt(inputIndex);
        }
        else 
        {
            std::wcout << L"INFO: Input '"<<inputName<<L"' not found.\n";
            args_error=true;
        }


        int outputIndex = getDeviceIndexFromString(outputs, outputName);
        if (outputIndex>=0)
        {
            std::wcout << L"INFO: Output found. ["<<outputIndex<<"] '"<<outputName<<L"'.\n";
            output=outputs.GetAt(outputIndex);
        }
        else 
        {
            std::wcout << L"INFO: Output '"<<outputName<<L"' not found.\n";
            args_error=true;
        }
    }

    // Dump usage and exit if necessary
    if (args_error) 
    {
        std::wcout << L"\n";
        std::wcout << L"----- Usage.\n";
        std::wcout << L"    EasyMidiRouter.exe                       - try to execute EasyMidiRouter.args.\n";
        std::wcout << L"    EasyMidiRouter.exe <file.args>           - execute specified <file.args>.\n";
        std::wcout << L"    EasyMidiRouter.exe <midi_src>            - midi src messages debug\n";
        std::wcout << L"    EasyMidiRouter.exe <midi_src> <midi_dst> - route midi src messages to dst\n";
        std::wcout << L"";
        std::wcout << L"    Notes:\n";
        std::wcout << L"        <midi_src/midi_dst>                  - could be index number, full name or name part\n";
        std::wcout << L"        EasyMidiRouter.args or <file.args>   - file format: line 0 <midi_src>, line 1 <midi_dst>\n";
        std::wcout << L"\n";
        return 0;
    }

    // Execute routing
    if (input)
    {
        auto portOp = MidiOutPort::FromIdAsync(output.Id());
        auto outPort = portOp.get();

        Windows::Foundation::IAsyncOperation<MidiInPort> inPortOp = MidiInPort::FromIdAsync(input.Id());
        MidiInPort inPort = inPortOp.get();
        inPort.MessageReceived([outPort](IMidiInPort const&, MidiMessageReceivedEventArgs const& args) 
        {
            IBuffer raw = args.Message().RawData();
            if (outPort)
                outPort.SendBuffer(raw);
                
            DataReader reader = DataReader::FromBuffer(raw);
            while (reader.UnconsumedBufferLength() > 0) 
            {
                uint8_t b = reader.ReadByte();
                std::cout << (b<=0xf?"0":"") << std::hex << static_cast<int>(b) << " ";

                static size_t line_output_count = 0;
                line_output_count++;
                if (line_output_count>=36)
                {
                    std::cout << "\n";
                    line_output_count=0;
                }
            }
        });

        std::cout << "\n";
        std::wcout << L"Routing messages from '"<<input.Name().c_str()<<"' to '"<<output.Name().c_str()<<"'";
        std::wcout << L"(Press CTRL+Q to exit)\n";
        while (true) {
            if (_kbhit()) {
                int ch = _getch();
                if (ch == 17) {
                    break;
                }
            }
        }

        std::wcout << L"Exiting...\n";
    }

    return 0;
}

//--------------------------------------------------------------------------------------------------------------------------
