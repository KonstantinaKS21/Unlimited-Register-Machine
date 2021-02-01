#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <sstream>
using namespace std;

class Machine;

class InvalidCommandException: exception
{
private:
	string msg_;

public:
	InvalidCommandException(const string& msg) : msg_(msg) {}
	~InvalidCommandException() {}

	string getMessage() const { return(msg_); }
};


//base class Operator
//Operator -> Instruction
//		   -> ZeroCommand, SetCommand, CopyCommand, ...
class Operator
{
protected:
	Machine* machine{ nullptr };

public:
	virtual void execute() = 0;
	virtual string toString() = 0;
	virtual ~Operator() {}
};


class Instruction: public Operator
{
public:
	unsigned index; //пореден номер
	static unsigned indexOfLast; 

	Instruction() : index(indexOfLast) { indexOfLast++; }
	unsigned getIndex() {
		return index;
	}

	virtual ~Instruction() {}
};
unsigned Instruction::indexOfLast{ 0 };


class Machine
{
private:
	void loadOrExecute(Operator* op, bool toLoad);

public:
	map<int, int> registry;

	vector<Operator*> operators;
	int currentOperatorIndex{ 0 };

	~Machine() {
		//registry manages its own memory
		for (auto i : operators)
		{
			delete i;
		}
	}
	
	void run();
	void parseCommands(vector<string>& line, bool toLoad = true);
	void erase();
};


//all of the instructions: INC, ZERO, MOVE, JUMP
class INC : public Instruction
{
private:
	unsigned index;

public:
	INC(Machine* inMachine, unsigned inIdex) : index(inIdex) {
		machine = inMachine;
	}

	void execute() override {
		machine->registry[index]++;
	}

	string toString() override {
		return "INC " + to_string(index);
	}
};

class ZERO : public Instruction
{
private:
	unsigned index;

public:
	ZERO(Machine* inMachine, unsigned inIndex) : index(inIndex) {
		machine = inMachine;
	}

	void execute() override {
		machine->registry[index] = 0;
	}

	string toString() override {
		return "ZERO " + to_string(index);
	}
};

class MOVE : public Instruction
{
private:
	unsigned indexFrom;
	unsigned indexTo;

public:
	MOVE(Machine* inMachine, unsigned inF, unsigned inT) : indexFrom(inF), indexTo(inT) {
		machine = inMachine;
	}

	void execute() override {
		machine->registry[indexTo] = machine->registry[indexFrom];
	}

	string toString() override {
		return "MOVE " + to_string(indexFrom) + " " + to_string(indexTo);
	}
};

class JUMP : public Instruction
{
private:
	unsigned indexToJump; //индекс на инструкция
	unsigned indexX; //индекс на регистър
	unsigned indexY; //индекс на регистър

public:
	JUMP(Machine* inMachine, unsigned inJ, unsigned inX = 0, unsigned inY = 0) :
		indexToJump(inJ), indexX(inX), indexY(inY)	{
		machine = inMachine;
	}

	void execute() override {
		if (machine->registry[indexX] == machine->registry[indexY]) {
			if (indexToJump >= indexOfLast || indexToJump < 0) {
				throw new InvalidCommandException("Invalid command!");
			}

			int step = 0;
			if (indexToJump < index) {
				step = -1;
			}
			else
				step = 1;

			int currentIndex = index;
			while (currentIndex != indexToJump) {
				Instruction* current = dynamic_cast<Instruction*>(machine->operators[currentIndex]);
				if (current != nullptr) {
					if (current->getIndex() == indexToJump) {
						machine->currentOperatorIndex = indexToJump - 1;
					}
				}
				else {
					currentIndex += step;
				}
			}
		}
	}

	string toString() override {
		return "JUMP " + to_string(indexX) + " " + to_string(indexY) + " " + to_string(indexToJump);
	}
};


class ZeroCommand : public Operator
{
private:
	unsigned x;
	unsigned y;

public:
	ZeroCommand(Machine* inMachine, unsigned inX, unsigned inY) :
		x(inX), y(inY) {
		machine = inMachine;
	}

	void execute() override {
		for (int i = x; i <= y; ++i) {
			machine->registry[i] = 0;
		}
	}

	string toString() override {
		return "/zero " + to_string(x) + " " + to_string(y);
	}
};


class SetCommand : public Operator
{
private:
	unsigned x;
	unsigned y;

public:
	SetCommand(Machine* inMachine, unsigned inX, unsigned inY) :
		x(inX), y(inY) {
		machine = inMachine;
	}

	void execute() override {
		machine->registry[x] = y;
	}

	string toString() override {
		return "/set " + to_string(x) + " " + to_string(y);
	}
};

class CopyCommand : public Operator
{
private:
	unsigned x;
	unsigned y;
	unsigned z;

public:
	CopyCommand(Machine* inMachine, unsigned inX, unsigned inY, unsigned inZ) :
		x(inX), y(inY), z(inZ) {
		machine = inMachine;
	}

	void execute() override {
		for (int i = 0; i < z; ++i) {
			machine->registry[y + i] = machine->registry[x + i];
		}
	}

	string toString() override {
		return "/copy " + to_string(x) + " " + to_string(y) + " " + to_string(z);
	}
};

class MemCommand : public Operator
{
private:
	unsigned x;
	unsigned y;

public:
	MemCommand(Machine* inMachine, unsigned inX, unsigned inY) :
		x(inX), y(inY) {
		machine = inMachine;
	}

	void execute() override {
		for (int i = x; i <= y; ++i) {
			cout << "registry[" << i << "]: " << machine->registry[i] << " " << endl;
		}
	}

	string toString() override {
		return "/mem " + to_string(x) + " " + to_string(y);
	}
};

class CodeCommand : public Operator
{
public:
	CodeCommand(Machine* inMachine) {
		machine = inMachine;
	}

	void execute() override {
		for (auto op : machine->operators) {
			cout << op->toString() << endl;
		}
	}

	string toString() override {
		return "/code ";
	}
};

class RunCommand : public Operator
{
public:
	RunCommand(Machine* inMachine) {
		machine = inMachine;
	}

	void execute() override {
		machine->run();
	}

	string toString() override {
		return "/run ";
	}
};



//Helper functions
vector<string> splitString(string& input_str, char delim) 
{
	vector<string> result;
	stringstream ss(input_str);
	string str; 
	while (getline(ss, str, delim))
	{
		result.push_back(str);
	}

	return result;
}

void readFile(string filename, Machine* machine)
{
	ifstream in(filename, ios::beg);
	string lineStr;
	while (getline(in, lineStr)) {
		cout << lineStr << endl;
		vector<string> line = splitString(lineStr, ' ');
		machine->parseCommands(line, true);
	}
	in.close();
}

void checkLength(vector<string>& line, int len)
{
	if (line.size() != len) {
		throw new InvalidCommandException("Wrong number of arguments for this command!");
	}
}


void mergeInstructions(Machine* oldM, Machine* newM) {
	//обхождаме новата машина, за да видим колко инструкции има
	int instructionsCounter = 0;
	for (auto i : newM->operators) {
		Instruction* current = dynamic_cast<Instruction*>(i);
		if (current != nullptr) instructionsCounter++;
	}

	//отместваме индекса на всяка инструкция в старата машина с горнополученото число
	for (auto i : oldM->operators) {
		Instruction* current = dynamic_cast<Instruction*>(i);
		//ако не се кастне до типа instruction*, ще върне nullptr
		if (current != nullptr) current->index += instructionsCounter;
	}

	//добавяме инструкциите от новата в старата най-отпред
	vector<Operator*> newOperators{ newM->operators };
	
	for (auto i : oldM->operators) {
		newOperators.push_back(i);
	}
}


void mergeMachines(Machine* oldM, Machine* newM) {
	int newMSize = newM->registry.rbegin()->first;
	//отместваме регистъра на стария мап
	map<int, int>::reverse_iterator reverseIt;

	for (reverseIt = oldM->registry.rbegin(); reverseIt != oldM->registry.rend(); ++reverseIt) {
		int registryKey = reverseIt->first;
		int registryValue = reverseIt->second;
		oldM->registry.erase(registryKey);
		oldM->registry.insert({ registryKey + newMSize, registryValue });
	}

	//добавяме регистъра на новия мап
	for (auto i : newM->registry) {
		oldM->registry.insert({ i.first, i.second });
	}

	mergeInstructions(oldM, newM);
}



//Machine methods
void Machine::run() {
	currentOperatorIndex = 0;
	while (currentOperatorIndex < operators.size()) {
		operators[currentOperatorIndex]->execute();
		currentOperatorIndex++;
	}
}

void Machine::erase() {
	operators.clear();
	registry.clear();
}

void Machine::loadOrExecute(Operator* op, bool toLoad) {
	if (toLoad)
		operators.push_back(op);
	else {
		op->execute();
		delete op;
	}
}


void Machine::parseCommands(vector<string>& line, bool toLoad) {
	if (line[0] == "ZERO") {
		checkLength(line, 2);
		loadOrExecute(new ZERO(this, stoi(line[1])), toLoad);
	}
	else if (line[0] == "INC") {
		checkLength(line, 2);
		loadOrExecute(new INC(this, stoi(line[1])), toLoad);
	}
	else if (line[0] == "MOVE") {
		checkLength(line, 3);
		unsigned x = stoi(line[1]);
		unsigned y = stoi(line[2]);
		loadOrExecute(new MOVE(this, x, y), toLoad);
	}
	else if (line[0] == "JUMP") {
		//JUMP с 1 аргумент
		if (line.size() == 2)
		{
			unsigned z = stoi(line[1]);
			loadOrExecute(new JUMP(this, 0, 0, z), toLoad);
		}
		//JUMP с 3 аргумента
		else {
			checkLength(line, 4);
			unsigned x = stoi(line[1]);
			unsigned y = stoi(line[2]);
			unsigned z = stoi(line[3]);
			loadOrExecute(new JUMP(this, x, y, z), toLoad);
		}
	}
	else if (line[0] == "/zero") {
		checkLength(line, 3);
		int x = stoi(line[1]);
		int y = stoi(line[2]);
		loadOrExecute(new ZeroCommand(this, x, y), toLoad);
	}

	else if (line[0] == "/set") {
		checkLength(line, 3);
		int x = stoi(line[1]);
		int y = stoi(line[2]);
		loadOrExecute(new SetCommand(this, x, y), toLoad);
	}

	else if (line[0] == "/copy") {
		checkLength(line, 4);
		int x = stoi(line[1]);
		int y = stoi(line[2]);
		int z = stoi(line[3]);
		loadOrExecute(new CopyCommand(this, x, y, z), toLoad);
	}

	else if (line[0] == "/mem") {
		checkLength(line, 3);
		int x = stoi(line[1]);
		int y = stoi(line[2]);
		loadOrExecute(new MemCommand(this, x, y), toLoad);
	}

	else if (line[0] == "/load") {
		checkLength(line, 2);
		erase();
		readFile(line[1], this);
	}

	else if (line[0] == "/run") {
		checkLength(line, 1);
		run();
	}

	else if (line[0] == "/add") {
		checkLength(line, 2);
		Machine* newMachine = new Machine();
		readFile(line[1], newMachine);
		mergeMachines(this, newMachine);
		delete newMachine;
	}

	else if (line[0] == "/quote") {
		line.erase(line.begin());
		parseCommands(line, true);
	}

	else if (line[0] == "/code") {
		loadOrExecute(new CodeCommand(this), toLoad);
	}

	else if (line[0] == "/comment") {}

	else if (line[0] == "/exit") return;

	else throw new InvalidCommandException("Invalid command!");
}




int main() {
	Machine* machine = new Machine();
	readFile("test.urm", machine);
	string input;

	while (input != "/exit") {
		cout << "$ ";
		getline(cin, input);
		vector<string> consoleLine = splitString(input, ' ');
		machine->parseCommands(consoleLine, false);
	}
	delete machine;
	return 0;
}

