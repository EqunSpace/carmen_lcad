
#include "Component/ModelManager.H"
#include "Component/ModelComponent.H"
#include "Component/ModelOptionDef.H"
#include "Robots/Beobot2/Hardware/BeoSonar.H"
#include <Ice/Ice.h>
#include <Ice/Service.h>
#include "Ice/RobotSimEvents.ice.H"
#include "Ice/SimEventsUtils.H"
#include "Ice/IceImageUtils.H"
#include "Ice/RobotBrainObjects.ice.H"


// ######################################################################
// ######################################################################
class RobotBrainServiceService : public Ice::Service {
  protected:
#if ICE_INT_VERSION >= 30402
    virtual bool start(int, char* argv[],int &);
#else
    virtual bool start(int, char* argv[]);
#endif	
    virtual bool stop() {
      if (itsMgr)
        delete itsMgr;
      return true;
    }

  private:
    Ice::ObjectAdapterPtr itsAdapter;
    ModelManager *itsMgr;
};

// ######################################################################
#if ICE_INT_VERSION >= 30402
bool RobotBrainServiceService::start(int argc, char* argv[],int& status)
#else
bool RobotBrainServiceService::start(int argc, char* argv[])
#endif	
{
  char adapterStr[255];

  //Create the topics
//  SimEventsUtils::createTopic(communicator(), "BeeStemMessageTopic");

  //Create the adapter
  int port = RobotBrainObjects::RobotBrainPort;
  bool connected = false;

  while(!connected)
  {
    try
    {
      LINFO("Trying Port:%d", port);
      sprintf(adapterStr, "default -p %i", port);
      itsAdapter = communicator()->createObjectAdapterWithEndpoints("BeeSonar",
          adapterStr);
      connected = true;
    }
    catch(Ice::SocketException)
    {
      port++;
    }
  }

  //Create the manager and its objects
  itsMgr = new ModelManager("BeeStemService");

  LINFO("Starting BeeSonar System");
  nub::ref<BeoSonar> sonar(new BeoSonar(0, *itsMgr, "BeeSonar", "BeeSonar"));
  LINFO("BeeSonar Created");
  itsMgr->addSubComponent(sonar);
  LINFO("BeeSonar Added As Sub Component");
  sonar->init(communicator(), itsAdapter);
  LINFO("BeeSonar Inited");

  itsMgr->parseCommandLine((const int)argc, (const char**)argv, "", 0, 0);

  itsAdapter->activate();

  itsMgr->start();

  return true;
}

// ######################################################################
int main(int argc, char** argv) {

  RobotBrainServiceService svc;
  return svc.main(argc, argv);
}


