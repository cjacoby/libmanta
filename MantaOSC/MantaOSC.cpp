#include "../core/Manta.h"
#include "../core/MantaExceptions.h"
#include <lo/lo.h>
#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <iostream>

void ErrorHandler(int num, const char *m, const char *path);
int LEDControlHandler(const char *path, const char *types,
      lo_arg **argv, int argc, void *data, void *instancePointer);
int LEDRowAndColumnHandler(const char *path,
      const char *types, lo_arg **argv, int argc, void *data,
      void *instancePointer);

class MantaOSC : public Manta
{
   public:
      MantaOSC();
      ~MantaOSC();
   private:
      virtual void PadEvent(int id, int value);
      virtual void SliderEvent(int id, int value);
      virtual void ButtonEvent(int id, int value);
      friend int LEDControlHandler(const char *path, const char *types,
            lo_arg **argv, int argc, void *data, void *instancePointer);
      friend int LEDRowAndColumnHandler(const char *path,
            const char *types, lo_arg **argv, int argc, void *data,
            void *instancePointer);
      lo_address OSCAddress;
      lo_server_thread OSCServerThread;
};


MantaOSC::MantaOSC()
{
   OSCAddress = lo_address_new("localhost","8000");
   OSCServerThread = lo_server_thread_new("8001", ErrorHandler);
   /* the callbacks are static, so we need to pass the "this" pointer so that object methods
    * can be called from within the callbacks */
   lo_server_thread_add_method(OSCServerThread, "/Manta/LEDControl", "si", LEDControlHandler, this);
   lo_server_thread_add_method(OSCServerThread, "/Manta/LED/Red", "sii", LEDRowAndColumnHandler, this);
   lo_server_thread_add_method(OSCServerThread, "/Manta/LED/Amber", "sii", LEDRowAndColumnHandler, this);
   lo_server_thread_start(OSCServerThread);
}

MantaOSC::~MantaOSC()
{
   lo_address_free(OSCAddress);
   lo_server_thread_free(OSCServerThread);
}

void MantaOSC::PadEvent(int id, int value)
{
   lo_send(OSCAddress, "/Manta/Pad", "ii", id, value);
}

void MantaOSC::SliderEvent(int id, int value)
{
   lo_send(OSCAddress, "/Manta/Slider", "ii", id, value);
}

void MantaOSC::ButtonEvent(int id, int value)
{
   lo_send(OSCAddress, "/Manta/Button", "ii", id, value);
}

int main(void)
{
   MantaOSC manta;
   do
   {
      try
      {
         manta.Connect();
      }
      catch(MantaNotFoundException &e)
      {
         std::cout << "Could not find an attached Manta. Retrying..." << std::endl;
         sleep(1);
      }
   } while(! manta.IsConnected());
   std::cout << "Manta Connected" << std::endl;
   try
   {
      while(1)
      {
         manta.HandleEvents();
      }
   }
   catch(MantaCommunicationException &e)
   {
      std::cout << "Communication with Manta interrupted, exiting..." << std::endl;
   }
   return 0;
}

int LEDControlHandler(const char *path, const char *types, lo_arg **argv, int argc,
      void *data, void *instancePointer)
{
   if(strcmp(&argv[0]->s, "PadAndButton") == 0)
   {
      static_cast<MantaOSC *>(instancePointer)->SetLEDControl(MantaOSC::PadAndButton, argv[1]->i);
   }
   else if(strcmp(&argv[0]->s, "Slider") == 0)
   {
      static_cast<MantaOSC *>(instancePointer)->SetLEDControl(MantaOSC::Slider, argv[1]->i);
   }
   else if(strcmp(&argv[0]->s, "Button") == 0)
   {
      static_cast<MantaOSC *>(instancePointer)->SetLEDControl(MantaOSC::Button, argv[1]->i);
   }
   else
   {
      return 1;
   }
   return 0;
}

int LEDRowAndColumnHandler(const char *path, const char *types, lo_arg **argv, int argc,
      void *data, void *instancePointer)
{
   MantaOSC::LEDColor color;
#if 0
   static unsigned int count = 0;
   std::cout << "LEDRowAndColumnHandler " << count++ << std::endl;
#endif
   if(strcmp(path, "/Manta/LED/Red") == 0)
   {
      color = MantaOSC::Red;
   }
   else if(strcmp(path, "/Manta/LED/Amber") == 0)
   {
      color = MantaOSC::Amber;
   }
   else
   {
      /* return unhandled, unrecognized color */
      return 1;
   }

   if(strcmp(&argv[0]->s, "row") == 0)
   {
      static_cast<MantaOSC *>(instancePointer)->SetLEDRow(color, argv[1]->i,
            static_cast<uint8_t>(argv[2]->i));
   }
   else if(strcmp(&argv[0]->s, "column") == 0)
   {
      static_cast<MantaOSC *>(instancePointer)->SetLEDColumn(color, argv[1]->i,
            static_cast<uint8_t>(argv[2]->i));
   }
   else if(strcmp(&argv[0]->s, "slider") == 0)
   {
      if(color == MantaOSC::Amber)
      {
         static_cast<MantaOSC *>(instancePointer)->SetSliderLEDs(argv[1]->i,
               static_cast<uint8_t>(argv[2]->i));
      }
      else
      {
         return 1;
      }
   }
   else if(strcmp(&argv[0]->s, "button") == 0)
   {
      static_cast<MantaOSC *>(instancePointer)->SetButtonLEDs(color, argv[1]->i,
            static_cast<uint8_t>(argv[2]->i));
   }
   else
   {
      /* return unhandled, not row or column */
      return 1;
   }
   return 0;
}

void ErrorHandler(int num, const char *msg, const char *path)
{
   std::cout << "liblo server error " << num << " in path " << path << 
      ": " << msg << std::endl;
}
