#include <stdio.h>
#include <carmen/carmen.h>
#include <carmen/can_dump_interface.h>


///////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                           //
// Publishers                                                                                //
//                                                                                           //
///////////////////////////////////////////////////////////////////////////////////////////////


static void
publish_can_dump_can_line_message(char *can_line)
{
//	carmen_can_dump_can_line_message message;

	can_line[strlen(can_line) - 1] = '\0'; // Apaga o '\n' no fim da string

	FILE *caco = fopen("can_dump.txt", "a");
	fprintf(caco, "%lf can_line %s\n", carmen_get_time(), can_line);
	fflush(caco);
	fclose(caco);

//	message.can_line = can_line;
//	message.timestamp = carmen_get_time();
//	message.host = carmen_get_host();

//	carmen_can_dump_publish_can_line_message(&message);
}
///////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                           //
// Handlers                                                                                  //
//                                                                                           //
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                           //
// Initializations                                                                           //
//                                                                                           //
///////////////////////////////////////////////////////////////////////////////////////////////



int
main (int argc, char **argv)
{
	carmen_ipc_initialize(argc, argv);
	carmen_can_dump_define_can_line_message();

	FILE *can_dump = popen("ssh -XC pi@192.168.0.13 'candump any'", "r");
	char line[1024];

	while (fgets(line, 1023, can_dump) != NULL)
		publish_can_dump_can_line_message(line);

	return (0);
}
