
typedef struct PCI_Transaction
{
        bool FRAMEn;
        __int32 AD;             //AD[31::00]
        __int8 CBE;     //C/BE[3::0]
        bool IRDYn;
        bool TRDYn;
        bool DEVSELn;
        bool IDSEL;
        bool PAR;
        bool GNTn;
        bool LOCKn;
        bool PERRn;
        bool REQn;
        bool SERRn;
        bool STOPn;
} pci_frame;

void show_menu();
void dump_pci_frame (pci_frame *frame_cap);
std::string getMessageType(int cbe);
int analyze_file(const char *filename);