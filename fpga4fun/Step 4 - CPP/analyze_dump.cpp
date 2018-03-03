#include <iostream>
#include <fstream>
#include <iomanip>

#include <vector>
#include <deque>

#include "analyze_dump.h"


int analyze_file(const char *filename) {

        std::cout << "\nParsing file";

        char block[8];
        int file_size, pos = 0;

        std::ifstream fin (filename, std::ios::in | std::ios::binary | std::ios::ate );
        fin.seekg(0, std::ios::end);
        file_size = fin.tellg();
        fin.seekg(0, std::ios::beg);
        std::vector<pci_frame> my_frames;

        if (fin.is_open())
        {
                while (fin.is_open()) {
                        pos = fin.tellg();
                        if ( pos + 8 <= file_size) {
                                //3fff 80f0 4408 0102
                                fin.read ( block, 8 );

                                pci_frame frame_cap;
                                frame_cap.AD = ( (block[5] & 0xFF) << 24 ) | ( (block[4] & 0xFF)
<< 16 ) | ( (block[3] & 0xFF) << 8 ) | ( (block[2] & 0xFF) );
                                frame_cap.CBE = (block[1] & 0xF0) >> 4;
                                // PCI_IRDYn, PCI_TRDYn, PCI_FRAMEn, PCI_DEVSELn,
                                frame_cap.IRDYn = (block[1] & 0x08);
                                frame_cap.TRDYn = (block[1] & 0x04);
                                frame_cap.FRAMEn = (block[1] & 0x02);
                                frame_cap.DEVSELn = (block[1] & 0x01);
                                // PCI_IDSEL, PCI_PAR, PCI_GNTn, PCI_LOCKn, PCI_PERRn, PCI_REQn, PCI_SERRn, PCI_STOPn
                                frame_cap.IDSEL = (block[1] & 0x80);
                                frame_cap.PAR = (block[1] & 0x40);
                                frame_cap.GNTn = (block[1] & 0x20);
                                frame_cap.LOCKn = (block[1] & 0x10);
                                frame_cap.PERRn = (block[1] & 0x08);
                                frame_cap.REQn = (block[1] & 0x04);
                                frame_cap.SERRn = (block[1] & 0x02);
                                frame_cap.STOPn = (block[1] & 0x01);

                                my_frames.push_back(frame_cap);
                                //dump_pci_frame (&frame_cap);
                        } else {
                                break;
                        }
                }

                fin.close();
        }

        std::cout << "\nTotal number of captured frames read: " << my_frames.size();

        bool in_frame = false;
        int num_find = 5;
        int i = 0;
        std::cout << "\n";
        std::deque<int> data_phases;
        for (int frame_num = 1; frame_num < 256; frame_num++) {
                pci_frame it2 = my_frames.at(frame_num);
                pci_frame prev_it = my_frames.at(frame_num-1);
                pci_frame *it = &it2;
                bool isAddressPhase = ( it->FRAMEn == false && prev_it.FRAMEn == true );
                bool isDataTransfer = ( in_frame == true && it->IRDYn == false &&
it->TRDYn == false );
                bool isFrameEnd = ( it->FRAMEn == true && it->TRDYn == true );
                if (isFrameEnd) {
                        // print all data
                        if (data_phases.size() > 0) {
                                std::cout << "\nData = [" << std::dec;
                                for (std::deque<int>::iterator it = data_phases.begin(); it !=
data_phases.end(); ++it) {
                                        std::cout << " " << *it;
                                }
                                std::cout << " ]";
                                data_phases.clear();
                        }
                        in_frame = false;
                } else if (isAddressPhase) {
                        in_frame = true;
                        data_phases.clear();
                        std::cout << "\nFrame #: " << frame_num;
                        std::cout << " AD [0x" << std::setw(4) << std::setfill('0')
                                                << std::hex << ((it->AD & 0xFFFF0000) >> 16)
                                                << " "
                                                << std::hex << ((it->AD & 0xFFFF)) << "]";
                        std::cout << " CBE [" << std::hex << (int)it->CBE << " = " <<
getMessageType((int)it->CBE).c_str() << "]";
                        std::cout << "\n";
                        if ( i++ > num_find )
                                break;
                }
                if (isDataTransfer) {
                        data_phases.push_back(it->AD);
                }
        }
        std::cout << "\n";

        return (0);
}

std::string getMessageType(int cbe)
{
        std::string messageType;
        switch (cbe)
        {
        case 2:
                messageType = "I/O Read";
                break;
        case 3:
                messageType = "I/O Write";
                break;
        case 6:
                messageType = "Memory Read";
                break;
        case 7:
                messageType = "Memory Write";
                break;
        case 0xA:
                messageType = "Configuration Read";
                break;
        case 0xB:
                messageType = "Configuration ReadWrite";
                break;
        case 0xC:
                messageType = "Memory Read Multiple";
                break;
        case 0xE:
                messageType = "Memory Read Line";
                break;
        case 0xF:
                messageType = "Memory Write and Invalidate";
                break;
        default:
                messageType = "Unknown: " + cbe;
                break;
        }
        return messageType;
}

void show_menu()
{
        std::cout << "\n+--------------------------------------------------+";
        std::cout << "\n| Enter Frame Number to view                       |";
        std::cout << "\n| q/Q to exit                                      |";
        std::cout << "\n+--------------------------------------------------+";
        std::cout << "\n";
}

void dump_pci_frame (pci_frame *frame_cap)
{
        std::cout << "\n";
        std::cout << "\nAD = " << std::setw(4) << std::setfill('0')
                << std::hex << ((frame_cap->AD & 0xFFFF0000) >> 16)
                << " "
                << std::hex << ((frame_cap->AD & 0xFFFF));
        std::cout << " CBE = " << std::hex << (int)frame_cap->CBE;

        // Display only what is asserted
        if (frame_cap->FRAMEn == false) std::cout << " FRAMEn";
        if (frame_cap->IRDYn == false) std::cout << " IRDYn";
        if (frame_cap->TRDYn == false) std::cout << " TRDYn";
        if (frame_cap->DEVSELn == false) std::cout << " DEVSELn";
        if (frame_cap->IDSEL == true) std::cout << " IDSEL";
        if (frame_cap->PAR == true) std::cout << " PAR";
        if (frame_cap->GNTn == false) std::cout << " GNTn";
        if (frame_cap->LOCKn == false) std::cout << " LOCKn";
        if (frame_cap->PERRn == false) std::cout << " PERRn";
        if (frame_cap->REQn == false) std::cout << " REQn";
        if (frame_cap->SERRn == false) std::cout << " SERRn";
        if (frame_cap->STOPn == false) std::cout << " STOPn";

        std::cout << "\n";
}