#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Devices.Midi.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/base.h>

#include <conio.h>
#include <iostream>

using namespace winrt;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Devices::Midi;
using namespace Windows::Storage::Streams;


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
            std::wcout << L"    [" << i << L"] " << device.Name().c_str() << std::endl;
        }
    }
}

int main(int argc, wchar_t* argv[])
{
    init_apartment();

    DeviceInformationCollection inputs = nullptr;
    enum_devices( true, inputs  );

    DeviceInformationCollection outputs = nullptr;
    enum_devices( false, outputs );

    std::wcout << L"\n";
    DeviceInformation input  = inputs .GetAt(0);
    DeviceInformation output = outputs.GetAt(0);
    
    bool args_error=false;
    if (args_error)
    {
        std::wcout << L"----- Usage.\n";
        std::wcout << L"    \n";

    }
    else
    {
        Windows::Foundation::IAsyncOperation<MidiInPort> portOp = MidiInPort::FromIdAsync(input.Id());
        MidiInPort port = portOp.get();
        port.MessageReceived([](IMidiInPort const&, MidiMessageReceivedEventArgs const& args) 
        {
            IBuffer raw = args.Message().RawData();
            DataReader reader = DataReader::FromBuffer(raw);

            std::cout << "MIDI: ";
            while (reader.UnconsumedBufferLength() > 0) 
            {
                std::cout << std::hex << static_cast<int>(reader.ReadByte()) << " ";
            }

            std::cout << "\n";
        });

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
