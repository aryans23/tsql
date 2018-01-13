#include "Helper.h"
#include "Query.h"

int main(int argc, char **argv)
{
	// Initialize the memory, disk and the schema manager
	MainMemory memory;
	Disk disk;
	SchemaManager schema_manager(&memory, &disk);
	disk.resetDiskIOs();
	disk.resetDiskTimer();
	resetMemory();
	clock_t start_time;
	start_time = clock();
	ifstream input;
	bool interactive_SQL = false;		// by default
	if (argv && argc < 1) {
		cout << "Unknown Error" << endl;
		return 0;
	}

	if (1 == argc) {
		cout << "\t\t\t\t---------------------- Usage ---------------------" << endl;
		cout << "\t\t\t\t    To read input file, type: ./Sql <filename>" << endl;
		cout << "\t\t\t\tTo go to Interactive Environment, type: ./Sql -I" << endl;
		cout << "\t\t\t\t           For help, type: ./Sql -help" << endl;
		cout << "\t\t\t\t--------------------------------------------------" << endl;
		return 0;
	} else if (2 == argc && string(argv[1]) == "-help") {
		cout << "\t\t\t\t---------------------- Usage ---------------------" << endl;
		cout << "\t\t\t\t    To read input file, type: ./Sql <filename>" << endl;
		cout << "\t\t\t\tTo go to Interactive Environment, type: ./Sql -I" << endl;
		cout << "\t\t\t\tExamples: " << endl;
		cout << "\t\t\t\tCREATE TABLE Contacts (pid INT, name STR20, score INT)" << endl;
		cout << "\t\t\t\tINSERT INTO Contacts (pid, name, score) VALUES (0, \"Peter Parker\", 83)" << endl;
		cout << "\t\t\t\tSELECT name, score FROM Contacts WHERE score > 50 ORDER BY score" << endl;
		cout << "\t\t\t\t--------------------------------------------------" << endl;
	} else if (2 == argc && (string(argv[1]) == "-I" || string(argv[1]) == "-i")) {
		interactive_SQL = true;
		cout << endl << "Entering Interactive Mode" << endl 
			<< "Type query and hit ENTER to execute" << endl 
			<< "Type \"quit/q\" to quit the program." << endl << endl;
	} else {
		string filename = argv[1];
		input.open(filename);
		if (!input.is_open()) {
			cout << "Cannot Open file: " << filename << endl;
			return 0;
		}
	}

	unsigned long int ios = 0;
	double curr_time = 0;
	string line;
	vector<string> words;
	int c = 1;

	while (true) {
		if (interactive_SQL) {
			cout << "[" << c << "] ";
			c++;						// incrementing command line
			getline(cin, line);
			if (line == "QUIT" || line == "quit" || line == "q") break;
			cout << endl;
		} else {
			if (!getline(input, line)) 
				break;	
		}
		cout << line << endl;
		if(line[0] == '#') continue;	// comments
		if(line.size() == 0) continue;
		words = split(line," ");		// extract each word into vector words
		resetMemory();					// prepare memory

		if (words[0] == "SELECT") {
			execSelect(words, schema_manager, memory);
		} else if (words[0] == "INSERT") {
			execInsert(words, line, schema_manager, memory);
		} else if (words[0] == "DELETE") {
			execDelete(words, schema_manager, memory);
		} else if (words[0] == "CREATE") {
			execCreate(words, schema_manager);
		} else if (words[0] == "DROP") {
			schema_manager.deleteRelation(words[2]);
		} else {
			cout << "Invalid/Unsupported SQL command!" << endl << endl;
			continue;
		}
		words.clear();
		cout << "Summary for the query ===> ";
		cout << "Disk I/Os: " << disk.getDiskIOs() - ios << ", ";
		cout << "Time elapsed: " << disk.getDiskTimer() - curr_time << " ms" << endl << endl;
		curr_time = disk.getDiskTimer();
		ios = disk.getDiskIOs();
	}

	if (!interactive_SQL) input.close();
	cout << " "<< endl;
	cout << "---------------------- Summary ----------------------" << endl;
	cout << "Total time elapsed = " << ((double)(clock()-start_time)/CLOCKS_PER_SEC*1000) << " ms" << endl;
	cout << "Total time spent for Disk I/Os = " << disk.getDiskTimer() << " ms" << endl;
	cout << "Total Disk I/Os = " << disk.getDiskIOs() << endl;
	cout << "-----------------------------------------------------" << endl;
	return 0;
}


