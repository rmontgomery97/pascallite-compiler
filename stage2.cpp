//Reese Montgomery
//CS4301
//Stage 2

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <stack>
#include <fstream>
#include <ctime>
#include <cmath>
#include <sstream>
#include <ctype.h>
#include <cstdlib>

using namespace std;

const int MAX_SYMBOL_TABLE_SIZE = 256;
enum storeType {INTEGER , BOOLEAN, PROG_NAME, UNKNOWN};
enum allocation {YES,NO};
enum modes {VARIABLE, CONSTANT};
string keyword[] = {"program", "const", "var", "integer", "boolean", "begin", "end", "true", "false", "not",
                    "mod", "div", "and", "or", "read", "write", "if", "then", "else", "repeat", "while", "do", "until"};
string AddLevelOps[] = {"+", "-", "or"};
string MultLevelOps[] = {"*", "div", "mod", "and"};
string RelLevelOps[] = {"=", "<", ">", "<=", ">=", "<>"};

struct entry  //define symbol table entry format
{
    string internalName;
    string externalName;
    storeType dataType;
    modes mode;
    string value; 
    allocation alloc;
    int units;
};
vector<entry> symbolTable;
stack<string> operatorStk;
stack<string> operandStk;
ifstream sourceFile;
ofstream listingFile,objectFile; 
string token;
char charac;
bool newLine = false;
string tempNewLine = "";
int numErrors = 0;
int lineNumber = 1;
int numBools = -1;
int numInts = -1;
int currentTempNo = -1;
int maxTempNo = -1;
int labelNo = -1;
int beginNo = 0;
const char END_OF_FILE = '$'; // arbitrary choice
time_t now = time (NULL);
string contentsOfAReg = "";

void CreateListingHeader();
void Parser();
void CreateListingTrailer();
void PrintSymbolTable();
char NextChar();
string NextToken();
void Prog();
void ProgStmt();
void Consts();
void Vars();
void IfStmt();
void WhileStmt();
void Else_pt();
void RepeatStmt();
void Express();
void Expresses();
void Term();
void Terms();
void Factor();
void Factors();
void Part();
void BeginEndStmt();
void AssignStmt();
void ReadStmt();
void WriteStmt();
void Insert(string, storeType, modes, string, allocation, int);
string WhichValue(string);
void PushOperator(string name)
{
    operatorStk.push(name);
}
void PushOperand(string name)
{ // if name is a literal, also create a symbol table enter for it
    //TODO: create symbol table entry
    bool isDefined;
    if (name == "true" || name == "not false")
    {
        isDefined = false;
        for (unsigned i = 0; i < symbolTable.size(); i++)
        {
            if (symbolTable[i].dataType == BOOLEAN && symbolTable[i].mode == CONSTANT 
                && symbolTable[i].value == "1")
            {
                isDefined = true;
                break;
            }
        }
        if (!isDefined)
        {
            Insert(name, BOOLEAN, CONSTANT, "1", YES, 1);
        }
    }
    else if (name == "false" || name == "not true")
    {
        isDefined = false;
        for (unsigned i = 0; i < symbolTable.size(); i++)
        {
            if (symbolTable[i].dataType == BOOLEAN && symbolTable[i].mode == CONSTANT 
                && symbolTable[i].value == "0")
            {
                isDefined = true;
                break;
            }
        }
        if (!isDefined)
        {
            Insert("false", BOOLEAN, CONSTANT, "0", YES, 1);
        }
    }
    istringstream inputString(name);
    int i;
    inputString >> i;
    
    if (inputString)
    {
        isDefined = false;
        for (unsigned i = 0; i < symbolTable.size(); i++)
        {
            if (symbolTable[i].externalName == name)
            {
                isDefined = true;
                break;
            }
        }
        if (!isDefined)
        {
            Insert(name, INTEGER, CONSTANT, name, YES, 1);
        }
    }
    
    operandStk.push(name);
}
string PopOperator()
{
    string op;
    
    if (!operatorStk.empty())
    {
        op = operatorStk.top();
        operatorStk.pop();
    }
    else
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": operator stack underflow";
        CreateListingTrailer();
        exit(1);
    }
    
    return op;
}
string PopOperand()
{
    string op;
    
    if (!operandStk.empty())
    {
        op = operandStk.top();
        operandStk.pop();
    }
    else
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": operand stack underflow";
        CreateListingTrailer();
        exit(1);
    }
    
    return op;
}
void EmitAdditionCode(string, string);
void EmitSubtractionCode(string, string);
void EmitMultiplicationCode(string, string);
void EmitDivisionCode(string, string);
void EmitAssignCode(string, string);
void EmitNegationCode(string);
void EmitNotCode(string);
void EmitModuloCode(string, string);
void EmitAndCode(string, string);
void EmitEqualsCode(string, string);
void EmitLessThanCode(string, string);
void EmitLessThanOrEqualsCode(string, string);
void EmitGreaterThanCode(string, string);
void EmitGreaterThanOrEqualsCode(string, string);
void EmitDoesNotEqualCode(string, string);
void EmitThenCode(string);
void EmitElseCode(string);
void EmitPostIfCode(string);
void EmitWhileCode();
void EmitDoCode(string);
void EmitPostWhileCode(string, string);
void EmitRepeatCode();
void EmitUntilCode(string, string);
void Code(string, string = "", string = "");
void ConstStmts();
void VarStmts();
void Express();
void ExecStmt();
void ExecStmts();
string Ids();
void FreeTemp();
string getTemp();
string getLabel();
string GenInternalName(storeType);
storeType WhichType(string);
bool isKeyword(string str) //Checks to see if a given string is a reserved keyword
{
    for (unsigned i = 0; i < (sizeof(keyword)/sizeof(keyword[0])) - 1; i++)
    {
        if (str == keyword[i])
        {
            return true;
        }
    }
    return false;
}

int main(int argc, char **argv)
{
    //this program is the stage0 compiler for Pascallite.  It will accept
    //input from argv[1], generating a listing to argv[2], and object code to
    //argv[3]
    if (argc != 4)
    {
        cerr << "Usage: " << argv[0] << " dataFile listingFile objectFile" << endl;
        

        return 1;
    }
    

    sourceFile.open(argv[1]);
    listingFile.open(argv[2]);
    objectFile.open(argv[3]);
    
    CreateListingHeader();

    Parser();

    CreateListingTrailer();

    //PrintSymbolTable();

    return 0;
}

void CreateListingHeader()
{
    /*  print "STAGE0:", names, DATE, TIME OF DAY;
        print "Line NO:", "SOURCE STATEMENT";
        //line numbers and source statements should be aligned under the headings */
    listingFile << setw(28) << left << "STAGE2:  Reese Montgomery";
    listingFile << ctime(&now) << endl;
    
    listingFile << "LINE NO.              SOURCE STATEMENT" << endl << endl;
    listingFile << setw(5) << right << lineNumber <<"|";

    lineNumber += 1;
    
}

void Parser()
{
    NextChar();
        //Charac must be initialized to the first character of the source file
    if (NextToken() != "program")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"program\" expected.";
        CreateListingTrailer();
        exit(1);
    }
        
    Prog();
    //parser implements the grammar rules, calling first rule */
}

void CreateListingTrailer()
{
    /* print "COMPILATION TERMINATED", "# ERRORS ENCOUNTERED"; */
    listingFile << endl << endl << "COMPILATION TERMINATED      " << numErrors << " ERRORS ENCOUNTERED" << endl;
}

void PrintSymbolTable()
{
    objectFile << setw(4) << "" << setw(2) << "" << setw(13) << "HLT" << endl; //add halt stmt and begin printing symbol table

    for (unsigned i = 0; i < symbolTable.size(); i++) // go through each entry and print each one
    {
        if (symbolTable[i].alloc == YES)
        {
            if (symbolTable[i].externalName[0] != 'T')
            {
                objectFile << setw(4) << symbolTable[i].internalName << "  ";
            }
            else 
            {
                objectFile << setw(4) << symbolTable[i].externalName << "  ";
            }
            
            if (symbolTable[i].mode == VARIABLE)
            {
                objectFile << setw(4) << "BSS";
                objectFile << setfill('0') << setw(4) << right << symbolTable[i].units;
            }
            else if (symbolTable[i].dataType == PROG_NAME)
            {
                objectFile << setw(4) << "DEC";
                objectFile << setfill('0') << setw(4) << right << symbolTable[i].units;
            }
            else
            {
                objectFile << setw(4) << "DEC";
                if (symbolTable[i].value[0] != '-')
                {
                    objectFile << setfill('0') << setw(4) << right << symbolTable[i].value;
                }
                else
                {
                    objectFile << "-" << setfill('0') << setw(3) << right << symbolTable[i].value.substr(1, symbolTable[i].value.length() - 1);
                }
            }                
            objectFile << setfill(' ') << setw(5) << left << "" << symbolTable[i].externalName << endl;
        }
    }
    objectFile << setw(6) << "" << "END STRT" << setw(5) << "" << endl;
    
    if (NextToken() != ".")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": \".\" expected.";
        CreateListingTrailer();
        exit(1);
    }
}

void Prog()
{
    //token should be "program"
    if (token != "program")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"program\" expected.";
        CreateListingTrailer();
        exit(1);
    }
    ProgStmt();
    if (token == "const") Consts();
    if (token == "var") Vars();
    if (token != "begin") 
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"begin\" expected.";
        CreateListingTrailer();
        exit(1);
    }    
    BeginEndStmt();
    NextToken();
    if (token[0] != (END_OF_FILE))
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": no text may follow \"end\"";
        CreateListingTrailer();
        exit(1);
    }
}

void ProgStmt() //token should be "program"
{
    string x;
    if (token != "program")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"program\" expected.";
        CreateListingTrailer();
        exit(1);
    }
    PushOperand(token);

    x = NextToken();

    if (isKeyword(token) || token == ";") //check for program name
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": program name expected";
        CreateListingTrailer();
        exit(1);
    }    

    if (NextToken() != ";") //make sure program name is followed by ";"
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": semicolon expected.";
        CreateListingTrailer();
        exit(1);
    }
    NextToken();
    
    Insert(x, PROG_NAME, CONSTANT, x, NO, 0); //insert prog name into symbol table
    Code(PopOperand());
}

void Consts() //token should be "const"
{
    if (token != "const")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"const\" expected.";
        CreateListingTrailer();
        exit(1);
    }

    NextToken();
    if (isKeyword(token)) //make sure token is not a reserved keyword
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": non-keyword identifier must follow \"const\"";
        CreateListingTrailer();
        exit(1);
    }
    ConstStmts(); //process const stmts
}

void Vars() //token should be "var"
{
    if (token != "var")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"var\" expected.";
        CreateListingTrailer();
        exit(1);
    }

    if (isKeyword(NextToken())) //make sure token following var is not a reserved keyword
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": non-keyword identifier must follow \"var\"";
        CreateListingTrailer();
        exit(1);
    }
    VarStmts(); //process var stmts
}

void BeginEndStmt() // token should be "begin"
{
    if (token != "begin")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"begin\" expected.";
        CreateListingTrailer();
        exit(1);
    }
    beginNo++;
    
    if (NextToken() != "end")
    {
        ExecStmts();
    }

    if (token != "end")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"end\" expected.";
        CreateListingTrailer();
        exit(1);
    }    
    Code("end");
    
    
    //NextToken();
}

void ExecStmts()
{
    ExecStmt();
    
    if (token == ";")
    {
        NextToken();
    }
    
    if (token != "end" && token != "until")
    {
        ExecStmts();
    }
}

void ExecStmt()
{   
    if (token == "end")
    {
        return;
    }
    else if (token == "begin")
    {
        BeginEndStmt();
    }
    else if (token == "read")
    {
        Code("read");
        if (NextToken() != ";")
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 <<": \";\" expected.";
            CreateListingTrailer();
            exit(1);
        }
    }
    else if (token == "write")
    {
        Code("write");
        if (NextToken() != ";")
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 <<": \";\" expected.";
            CreateListingTrailer();
            exit(1);
        }
        
    }
    else if (token == "if")
    {
        IfStmt();
    }
    else if (token == "while")
    {
        WhileStmt();
    }
    else if (token == "repeat")
    {
        RepeatStmt();
    }
    else if (token == ";")
    {
        //NULL stmt
    }
    else 
    {
        AssignStmt(); 
        if (token != ";")
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 <<": \";\" expected.";
            CreateListingTrailer();
            exit(1);
        }
    }
}

void ReadStmt() //token should be "read"
{
    string x, inName, id = "";
    
    if (NextToken() != "(")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": \"(\" expected.";
        CreateListingTrailer();
        exit(1);
    }
    NextToken();
    
    x = Ids();
    
    for (unsigned i = 0; i < x.length(); i++)
    {
        if (x[i] != ',')
        {
            id += x[i];
        }
        if (x[i] == ',' || i == x.length()-1)
        {
            for (unsigned j = 0; j < symbolTable.size(); j++)
            {
                if (id == symbolTable[j].externalName)
                {
                    if (symbolTable[j].mode == CONSTANT)
                    {
                        numErrors += 1;
                        listingFile << endl << "Error: Line " << lineNumber - 1 << ": cannot read in constant";
                        CreateListingTrailer();
                        exit(1);
                    }
                    inName = symbolTable[j].internalName;
                    break;
                }
            }
            if (inName.empty())
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
                CreateListingTrailer();
                exit(1);
            }
            
            objectFile << setw(6) << "";
            objectFile << "RDI " << setw(3) << left << inName;
            objectFile << setw(6) << "" << "read(" << id << ")" << endl;
            id = "";
        }
    }
    
    if (token != ")")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": \")\" expected.";
        CreateListingTrailer();
        exit(1);
    }
}

void WriteStmt() //token should be "write"
{
    string x, inName, id = "";
    
    if (NextToken() != "(")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": \"(\" expected.";
        CreateListingTrailer();
        exit(1);
    }
    NextToken();
    
    x = Ids();
    
    for (unsigned i = 0; i < x.length(); i++)
    {
        if (x[i] != ',')
        {
            id += x[i];
        }
        if (x[i] == ',' || i == x.length()-1)
        {
            for (unsigned j = 0; j < symbolTable.size(); j++)
            {
                if (id == symbolTable[j].externalName)
                {
                    inName = symbolTable[j].internalName;
                    break;
                }
            }
            
            istringstream inputString(id);
            int i;
            inputString >> i;
            
            if (inputString)
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 <<": non-keyword identifier expected";
                CreateListingTrailer();
                exit(1);
            }
            
            if (inName.empty())
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 <<": reference to undefined variable";
                CreateListingTrailer();
                exit(1);
            }
            
            objectFile << setw(6) << "";
            objectFile << "PRI " << setw(3) << left << inName;
            objectFile << setw(6) << "" << "write(" << id << ")" << endl;
            id = "";
        }
    }
    
    if (token != ")")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": \")\" expected.";
        CreateListingTrailer();
        exit(1);
    }
}

void IfStmt()//token should be "if"
{
    if (token != "if")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"if\" expected.";
        CreateListingTrailer();
        exit(1);
    }
    
    NextToken();
    Express();
    
    if (token != "then")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"then\" expected.";
        CreateListingTrailer();
        exit(1);
    }
    Code("then", PopOperand());
    NextToken();
    
    ExecStmt();
    
    if (NextToken() == "else")
    {
        Else_pt();
    }
    
    Code("post_if", PopOperand());
}

void Else_pt()
{
    if (token != "else")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"else\" expected.";
        CreateListingTrailer();
        exit(1);
    }
    
    Code("else", PopOperand());
    NextToken();
    ExecStmt();
}

void WhileStmt()
{
    if (token != "while")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"while\" expected.";
        CreateListingTrailer();
        exit(1);
    }
    
    Code("while");
    NextToken();
    
    Express();
    
    if (token != "do")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"do\" expected.";
        CreateListingTrailer();
        exit(1);
    }
    Code("do", PopOperand());
    NextToken();
    ExecStmt();
    string rhs = PopOperand();
    string lhs = PopOperand();
    Code("post_while", rhs, lhs);
    NextToken();
}

void RepeatStmt()
{
    if (token != "repeat")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"repeat\" expected.";
        CreateListingTrailer();
        exit(1);
    }
    Code("repeat");
    NextToken();
    ExecStmts();
    if (token != "until")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": keyword \"until\" expected.";
        CreateListingTrailer();
        exit(1);
    }
    NextToken();
    Express();
    string rhs = PopOperand();
    string lhs = PopOperand();
    Code("until", rhs, lhs);
    NextToken();
    
}

void Express()
{
    Term();
    Expresses();
}

void Expresses()
{
    if (token == ";" || token == ")")
        return;
    
    for (unsigned i = 0; i < sizeof(RelLevelOps)/sizeof(RelLevelOps[0]); i++)
    {
        if (token == RelLevelOps[i])
        {
            PushOperator(token);
            NextToken();
            Term();
            string rhs = PopOperand();
            string lhs = PopOperand();
            Code(PopOperator(), rhs, lhs);
            Expresses();
            break;
        }
    }
}

void Term()
{
    Factor();
    Terms();
}

void Terms()
{    
    if (token == ";" || token == ")")
        return;
    
    
    for (unsigned i = 0; i < sizeof(AddLevelOps)/sizeof(AddLevelOps[0]); i++)
    {
        if (token == AddLevelOps[i])
        {
            PushOperator(token);
            NextToken();
            Factor();
            string rhs = PopOperand();
            string lhs = PopOperand();
            Code(PopOperator(), rhs, lhs);
            Terms();
            break;
        }
    }
    
}

void Factor()
{
    Part();
    Factors();
}

void Factors()
{
    if (token == ";" || token == ")")
    {
        return;
    }        
    
    for (unsigned i = 0; i < sizeof(MultLevelOps)/sizeof(MultLevelOps[0]); i++)
    {
        if (token == MultLevelOps[i])
        {
            PushOperator(token);
            NextToken();
            Part();
            string rhs = PopOperand();
            string lhs = PopOperand();
            Code(PopOperator(), rhs, lhs);
            Factors();
            break;
        }
    }
    
}

void Part()
{    
    bool isDefined = false;
    if (token == ";" || token == ")")
    {
        return;
    }
    if (token == "not")
    {
        if (NextToken() == "(")
        {
            NextToken();
            Express();
            if (token != ")")
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 <<": \")\" expected";
                CreateListingTrailer();
                exit(1);
            }
            Code("not", PopOperand());
            NextToken();
        }
        else if (token == "true")
        {
            PushOperand("false");
            
            for (unsigned i = 0; i < symbolTable.size(); i++)
            {
                if (symbolTable[i].externalName == "FALS")
                {
                    isDefined = true;
                    break;
                }
            }
            if (!isDefined)
            {
                Insert("FALS", BOOLEAN, CONSTANT, "0", YES, 1);
            }
            NextToken();
        }
        else if (token == "false")
        {
            PushOperand("true");
            for (unsigned i = 0; i < symbolTable.size(); i++)
            {
                if (symbolTable[i].externalName == "TRUE")
                {
                    isDefined = true;
                    break;
                }
            }
            if (!isDefined)
            {
                Insert("TRUE", BOOLEAN, CONSTANT, "1", YES, 1);
            }
            NextToken();
        }
        else
        {
            for (unsigned i = 0; i < symbolTable.size(); i++)
            {
                if (token == symbolTable[i].externalName)
                {
                    Code("not", token);
                    NextToken();
                    break;
                }
            }
        }
    }
    else if (token == "+")
    {
        istringstream inputString(NextToken());
        int i;
        inputString >> i;
        
        if (token == "(")
        {
            NextToken();
            Express();
            if (token != ")")
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 <<": \")\" expected";
                CreateListingTrailer();
                exit(1);
            }
            NextToken();
        }
        else if (inputString || !isKeyword(token))
        {
            PushOperand(token);
            NextToken();
        }
    }
    else if (token == "-")
    {   
        istringstream inputString(NextToken());
        int i;
        inputString >> i;
        
        if (inputString)
        {
            PushOperand("-" + token);
            NextToken();
        }
        else if (token == "(")
        {
            NextToken();
            Express();
            if (token != ")")
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 <<": \")\" expected";
                CreateListingTrailer();
                exit(1);
            }
            Code("neg", PopOperand());
            NextToken();
        }
        else if (!isKeyword(token))
        {
            Code("neg", token);
            NextToken();
        }        
    }
    else if (token == "(")
    {
        NextToken();
        Express();
        NextToken();
    }
    else if (token == "true")
    {
        isDefined = false;
        for (unsigned i = 0; i < symbolTable.size(); i++)
        {
            if (symbolTable[i].dataType == BOOLEAN && symbolTable[i].mode == CONSTANT && symbolTable[i].value == "1")
            {
                isDefined = true;
                PushOperand(symbolTable[i].externalName);
                break;
            }
        }
        if (!isDefined)
        {
            Insert(token, BOOLEAN, CONSTANT, "1", YES, 1);
            PushOperand(token);
        }
        NextToken();
    }
    else if (token == "false")
    {
        isDefined = false;
        for (unsigned i = 0; i < symbolTable.size(); i++)
        {
            if (symbolTable[i].dataType == BOOLEAN && symbolTable[i].mode == CONSTANT && symbolTable[i].value == "0")
            {
                isDefined = true;
                PushOperand(symbolTable[i].externalName);
                break;
            }
        }
        if (!isDefined)
        {
            Insert(token, BOOLEAN, CONSTANT, "0", YES, 1);
            PushOperand(token);
        }
        NextToken();
    }
    else if(!isKeyword(token))
    {
        PushOperand(token);
        NextToken();
    }
}

void AssignStmt()
{
    if (isKeyword(token))
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": non-keyword identifier, \"read\", \"write\", \"if\", \"while\", \"repeat\", \";\", or \"begin\" expected";
        CreateListingTrailer();
        exit(1);
    }
    PushOperand(token);
    
    if (NextToken() != ":=")
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": \":=\" expected";
        CreateListingTrailer();
        exit(1);
    }
    PushOperator(token);
    NextToken();
    
    Express();        
    string rhs = PopOperand();
    string lhs = PopOperand();
    Code(PopOperator(), rhs, lhs);
}

void EmitAdditionCode(string operand1, string operand2)
{
    string inName1, inName2;
    
    for (unsigned i = 0; i < symbolTable.size(); i++) // make sure each operand is an integer
    {
        if (symbolTable[i].externalName == operand1 || symbolTable[i].externalName == operand2)
        {
            if (symbolTable[i].dataType != INTEGER) // if it isnt an integer, process error
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 << ": both operands must be an integer";
                CreateListingTrailer();
                exit(1);
            }
        } 
    }
    
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand2 == symbolTable[i].externalName)
        {
            inName2 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName2.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand1 == symbolTable[i].externalName)
        {
            inName1 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName1.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    
    if (contentsOfAReg[0] == 'T')
    {
        if (contentsOfAReg != operand1 && contentsOfAReg != operand2) 
        {                         
            string inName3;
            for (unsigned i = 0; i < symbolTable.size(); i++)
            {
                if (contentsOfAReg == symbolTable[i].externalName)
                {
                    inName3 = symbolTable[i].internalName;
                }
            }                                                                                       //if a reg holds a temp not
                                                                                                    //operand 1 or 2, deassign it,
            objectFile << setw(4) << "" << setw(2) << "" << "STA " << setw(4) <<left << inName3;    //store the temp into memory,
            objectFile << setw(5) << "" << "deassign AReg" << endl;                                //and change the alloc entry in the table to yes
            
            for (unsigned i = 0; i < symbolTable.size(); i++)
            {
                if (symbolTable[i].externalName ==  contentsOfAReg)
                {
                    symbolTable[i].alloc = YES;
                    break;
                }
            }
            
            contentsOfAReg = "";
        }
    }

    if (contentsOfAReg != operand1 && contentsOfAReg != operand2) // if neither operand is in A reg, load operand2 into AReg
    {
        contentsOfAReg = "";
        objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << left << inName2;
        objectFile << setw(5) << "" << endl;
    }
    
    
    if (contentsOfAReg == operand1)
    {
        objectFile << setw(4) << "" << setw(2) << "" << "IAD " << setw(4) << left << inName2; // emit code to perform addition
    }
    else
    {
        objectFile << setw(4) << "" << setw(2) << "" << "IAD " << setw(4) << left << inName1; // emit code to perform addition
    }
    objectFile << setw(5) << "" << operand2 << " + " << operand1 << endl;
    
    if (operand1[0] == 'T') // if the operand is a temp, free it
    {
        FreeTemp();
    }
    if (operand2[0] == 'T')
    {
        FreeTemp();
    }
    
    contentsOfAReg = getTemp();
    
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName ==  contentsOfAReg)
        {
            symbolTable[i].dataType = INTEGER;
            break;
        }
    }
    
    PushOperand(contentsOfAReg);
}

void EmitSubtractionCode(string operand1, string operand2)
{
    string inName1, inName2;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand2 == symbolTable[i].externalName)
        {
            inName2 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName2.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand1 == symbolTable[i].externalName)
        {
            inName1 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName1.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    
    for (unsigned i = 0; i < symbolTable.size(); i++) // make sure each operand is an integer
    {
        if (symbolTable[i].externalName == operand1 || symbolTable[i].externalName == operand2)
        {
            if (symbolTable[i].dataType != INTEGER) // if it isnt an integer, process error
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 << ": illegal type";
                CreateListingTrailer();
                exit(1);
            }
        } 
    }
    if (contentsOfAReg[0] == 'T' && contentsOfAReg != operand2)                                 //if a reg holds a temp not
    {                                                                                           //operand 2, deassign it,
        objectFile << setw(4) << "" << setw(2) << "" << "STA " << setw(4) << contentsOfAReg;    //store the temp into memory,
        objectFile << setw(5) << "" << "deassign AReg" << endl;                                //and change the alloc entry in the table to yes
        
        for (unsigned i = 0; i < symbolTable.size(); i++)
        {
            if (symbolTable[i].externalName ==  contentsOfAReg)
            {
                symbolTable[i].alloc = YES;
                break;
            }
        }
        
        contentsOfAReg = "";
    }
    
    if (contentsOfAReg != operand2)
    {
        contentsOfAReg = "";
        objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << inName2;
        objectFile << setw(5) << "" << endl;
    }
    
    objectFile << setw(4) << "" << setw(2) << "" << "ISB " << setw(4) << inName1; // emit code to perform subtraction
    objectFile << setw(5) << "" << operand2 << " - " << operand1 << endl;
    
    if (operand1[0] == 'T') // if either operand is a temp, free it
    {
        FreeTemp();
    }
    if (operand2[0] == 'T')
    {
        FreeTemp();
    }
    
    contentsOfAReg = getTemp();
    
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName ==  contentsOfAReg)
        {
            symbolTable[i].dataType = INTEGER;
            break;
        }
    }
    
    PushOperand(contentsOfAReg);    
}

void EmitDivisionCode(string operand1, string operand2)
{
    string inName1, inName2;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand2 == symbolTable[i].externalName)
        {
            inName2 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName2.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand1 == symbolTable[i].externalName)
        {
            inName1 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName1.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    
    
    for (unsigned i = 0; i < symbolTable.size(); i++) // make sure each operand is an integer
    {
        if (symbolTable[i].externalName == operand1 || symbolTable[i].externalName == operand2)
        {
            if (symbolTable[i].dataType != INTEGER) // if it isnt an integer, process error
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 << ": illegal type";
                CreateListingTrailer();
                exit(1);
            }
        } 
    }
    if (contentsOfAReg[0] == 'T' && contentsOfAReg != operand2)                                 //if a reg holds a temp not
    {                                                                                           //operand 2, deassign it,
        objectFile << setw(4) << "" << setw(2) << "" << "STA " << setw(4) << contentsOfAReg;    //store the temp into memory,
        objectFile << setw(5) << "" << "deassign AReg" << endl;                                //and change the alloc entry in the table to yes
        
        for (unsigned i = 0; i < symbolTable.size(); i++)
        {
            if (symbolTable[i].externalName ==  contentsOfAReg)
            {
                symbolTable[i].alloc = YES;
                break;
            }
        }
        
        contentsOfAReg = "";
    }
    
    if (contentsOfAReg != operand2)
    {
        contentsOfAReg = "";
        objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << inName2;
        objectFile << setw(5) << "" << endl;
    }
    
    objectFile << setw(4) << "" << setw(2) << "" << "IDV " << setw(4) << inName1; // emit code to perform division
    objectFile << setw(5) << "" << operand2 << " div " << operand1 << endl;
    
    if (operand1[0] == 'T') // if the operand is a temp, free it
    {
        FreeTemp();
    }
    if (operand2[0] == 'T')
    {
        FreeTemp();
    }
    
    contentsOfAReg = getTemp();
    
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName ==  contentsOfAReg)
        {
            symbolTable[i].dataType = INTEGER;
            break;
        }
    }
    
    PushOperand(contentsOfAReg);    
}

void EmitModuloCode(string operand1, string operand2)
{
    string temp, inName1, inName2;
    
    for (unsigned i = 0; i < symbolTable.size(); i++) // make sure each operand is an integer
    {
        if (symbolTable[i].externalName == operand1 || symbolTable[i].externalName == operand2)
        {
            if (symbolTable[i].dataType != INTEGER) // if it isnt an integer, process error
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 << ": illegal type";
                CreateListingTrailer();
                exit(1);
            }
        } 
    }
    if (contentsOfAReg[0] == 'T' && contentsOfAReg != operand2)                                 //if a reg holds a temp not
    {                                                                                           //operand 2, deassign it,
        objectFile << setw(4) << "" << setw(2) << "" << "STA " << setw(4) << contentsOfAReg;    //store the temp into memory,
        objectFile << setw(5) << "" << "deassign AReg" << endl;                                //and change the alloc entry in the table to yes
        
        for (unsigned i = 0; i < symbolTable.size(); i++)
        {
            if (symbolTable[i].externalName ==  contentsOfAReg)
            {
                symbolTable[i].alloc = YES;
                break;
            }
        }
        
        contentsOfAReg = "";
    }
    
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand2 == symbolTable[i].externalName)
        {
            inName2 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName2.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand1 == symbolTable[i].externalName)
        {
            inName1 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName1.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    
    if (contentsOfAReg != operand2)
    {
        contentsOfAReg = "";
        objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << inName2;
        objectFile << setw(5) << "" << endl;
    }
    
    
    objectFile << setw(4) << "" << setw(2) << "" << "IDV " << setw(4) << inName1; // emit code to perform division
    objectFile << setw(5) << "" << operand2 << " mod " << operand1 << endl;
    
    if (operand1[0] == 'T') // if the operand is a temp, free it
    {
        FreeTemp();
    }
    if (operand2[0] == 'T')
    {
        FreeTemp();
    }
    
    temp = getTemp();
    objectFile << setw(4) << "" << setw(2) << "" << "STQ " << setw(4) << temp << "     ";
    objectFile << "store remainder in memory" << endl;    
    objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << temp << "     ";
    objectFile << "load remainder from memory"<< endl;
    
    contentsOfAReg = temp;
    
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName ==  temp)
        {
            symbolTable[i].alloc = YES;
            break;
        }
    }
    
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName ==  temp)
        {
            symbolTable[i].dataType = INTEGER;
            break;
        }
    }
    
    PushOperand(temp);    
}

void EmitMultiplicationCode(string operand1, string operand2)
{
    string inName1, inName2;
    for (unsigned i = 0; i < symbolTable.size(); i++) // make sure each operand is an integer
    {
        if (symbolTable[i].externalName == operand1 || symbolTable[i].externalName == operand2)
        {
            if (symbolTable[i].dataType != INTEGER) // if it isnt an integer, process error
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 << ": illegal type";
                CreateListingTrailer();
                exit(1);
            }
        } 
    }
    
    if (contentsOfAReg[0] == 'T')
    {
        if (contentsOfAReg != operand1 && contentsOfAReg != operand2)                               //if a reg holds a temp not
        {                                                                                           //operand 1 or 2, deassign it,
            objectFile << setw(4) << "" << setw(2) << "" << "STA " << setw(4) << contentsOfAReg;    //store the temp into memory,
            objectFile << setw(5) << "" << "deassign AReg" << endl;                                //and change the alloc entry in the table to yes
            
            for (unsigned i = 0; i < symbolTable.size(); i++)
            {
                if (symbolTable[i].externalName ==  contentsOfAReg)
                {
                    symbolTable[i].alloc = YES;
                    break;
                }
            }
            
            contentsOfAReg = "";
        }
    }
    
    if (operand2[0] != 'T')
    {
        for (unsigned i = 0; i < symbolTable.size(); i++) // get internal name for op2
        {
            if (operand2 == symbolTable[i].externalName)
            {
                inName2 = symbolTable[i].internalName;
                break;
            }
        }
    }
    else 
    {
        inName2 = operand2;
    }
    if (inName2.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    if (operand1[0] != 'T')
    {
        for (unsigned i = 0; i < symbolTable.size(); i++) //get internal name for op1
        {
            if (operand1 == symbolTable[i].externalName)
            {
                inName1 = symbolTable[i].internalName;
                break;
            }
        }
    }
    else 
    {
        inName1 = operand1;
    }
    if (inName1.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    
    if (contentsOfAReg != operand1 && contentsOfAReg != operand2) // if neither operand is in A reg, load operand2 into AReg
    {
        contentsOfAReg = "";
        objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << left << inName2;
        objectFile << setw(5) << "" << endl;
    }
    
    
    
    objectFile << setw(4) << "" << setw(2) << "" << "IMU " << setw(4) << left << inName1; // emit code to perform addition
    objectFile << setw(5) << "" << operand2 << " * " << operand1 << endl;
    
    if (operand1[0] == 'T') // if the operand is a temp, free it
    {
        FreeTemp();
    }
    if (operand2[0] == 'T')
    {
        FreeTemp();
    }
    
    contentsOfAReg = getTemp();
    
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName ==  contentsOfAReg)
        {
            symbolTable[i].dataType = INTEGER;
            break;
        }
    }
    
    PushOperand(contentsOfAReg);
}

void EmitAndCode(string operand1, string operand2)
{
    string inName1, inName2;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand2 == symbolTable[i].externalName)
        {
            inName2 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName2.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand1 == symbolTable[i].externalName)
        {
            inName1 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName1.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    
    for (unsigned i = 0; i < symbolTable.size(); i++) // make sure each operand is a boolean
    {
        if (symbolTable[i].externalName == operand1 || symbolTable[i].externalName == operand2)
        {
            if (symbolTable[i].dataType != BOOLEAN) // if it isnt a boolean, process error
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 << ": illegal type";
                CreateListingTrailer();
                exit(1);
            }
        } 
    }
    
    if (contentsOfAReg[0] == 'T' && (contentsOfAReg != operand1 && contentsOfAReg != operand2)) //if a reg holds a temp not
    {                                                                                           //operand 1 or 2, deassign it,
        objectFile << setw(4) << "" << setw(2) << "" << "STA " << setw(4) << left << contentsOfAReg;    //store the temp into memory,
        objectFile << setw(5) << "" << "deassign AReg" << endl;                                //and change the alloc entry in the table to yes
        
        for (unsigned i = 0; i < symbolTable.size(); i++)
        {
            if (symbolTable[i].externalName ==  contentsOfAReg)
            {
                symbolTable[i].alloc = YES;
                break;
            }
        }
        
        contentsOfAReg = "";
    }
    
    if (contentsOfAReg != operand1 && contentsOfAReg != operand2) // if neither operand is in A reg, load operand2 into AReg
    {
        contentsOfAReg = "";
        objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << left << inName1;
        objectFile << setw(5) << "" << endl;
    }
    
    objectFile << setw(4) << "" << setw(2) << "" << "IMU " << setw(4) << inName2; // emit code to perform addition
    objectFile << setw(5) << "" << operand2 << " and " << operand1 << endl;
    
    if (operand1[0] == 'T') // if the operand is a temp, free it
    {
        FreeTemp();
    }
    if (operand2[0] == 'T')
    {
        FreeTemp();
    }
    
    contentsOfAReg = getTemp();
    
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName ==  contentsOfAReg)
        {
            symbolTable[i].dataType = BOOLEAN;
            break;
        }
    }
    
    PushOperand(contentsOfAReg);
}

void EmitOrCode(string operand1, string operand2)
{
    string inName1, inName2;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand2 == symbolTable[i].externalName)
        {
            inName2 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName2.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand1 == symbolTable[i].externalName)
        {
            inName1 = symbolTable[i].internalName;
            break;
        }
    }
    /* if (inName1.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    } */
    
    for (unsigned i = 0; i < symbolTable.size(); i++) // make sure each operand is a boolean
    {
        if (symbolTable[i].externalName == operand1 || symbolTable[i].externalName == operand2)
        {
            if (symbolTable[i].dataType != BOOLEAN) // if it isnt a boolean, process error
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 << ": illegal type";
                CreateListingTrailer();
                exit(1);
            }
        } 
    }
    
    if (contentsOfAReg[0] == 'T' && (contentsOfAReg != operand1 && contentsOfAReg != operand2)) //if a reg holds a temp not
    {                                                                                           //operand 1 or 2, deassign it,
        objectFile << setw(4) << "" << setw(2) << "" << "STA " << setw(4) << contentsOfAReg;    //store the temp into memory,
        objectFile << setw(5) << "" << "deassign AReg" << endl;                                //and change the alloc entry in the table to yes
        
        for (unsigned i = 0; i < symbolTable.size(); i++)
        {
            if (symbolTable[i].externalName ==  contentsOfAReg)
            {
                symbolTable[i].alloc = YES;
                break;
            }
        }
        
        contentsOfAReg = "";
    }
    
    if (contentsOfAReg[0] != 'T' && (contentsOfAReg != operand1 && contentsOfAReg != operand2))
    {
        contentsOfAReg = "";
        objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << inName2;
        objectFile << setw(5) << "" << endl;
        contentsOfAReg = operand2;
    }
    
    if (contentsOfAReg == operand2)
    {
        objectFile << setw(4) << "" << setw(2) << "" << "IAD " << setw(4) << inName1; // emit code to perform addition
    }
    else
    {
        objectFile << setw(4) << "" << setw(2) << "" << "IAD " << setw(4) << inName2; // emit code to perform addition
    }
    objectFile << setw(5) << "" << operand2 << " or " << operand1 << endl;
    string label = getLabel();
    objectFile << setw(4) << "" << setw(2) << "" << "AZJ " << setw(4) << left << label << "+1   " << endl;
    objectFile << setw(4) << label << setw(2) << "" << "LDA " << setw(4) << left << "TRUE     " << endl;
    
    bool isDefined = false;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == "TRUE")
        {
            isDefined = true;
            break;
        }
    }
    if (!isDefined)
    {
        Insert("TRUE", BOOLEAN, CONSTANT, "1", YES, 1);
    }    
    
    if (operand1[0] == 'T') // if either operand is a temp, free it
    {
        FreeTemp();
    }
    if (operand2[0] == 'T')
    {
        FreeTemp();
    }
    
    contentsOfAReg = getTemp();
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName ==  contentsOfAReg)
        {
            symbolTable[i].dataType = BOOLEAN;
            break;
        }
    }
    
    PushOperand(contentsOfAReg);    
}

void EmitAssignCode(string operand1, string operand2)
{
    storeType op1Type;
    string inName1, inName2;
    if (operand1 == "false")
    {
        operand1 = "FALS";
    }

    
    for (unsigned i = 0; i < symbolTable.size(); i++) // get internal names of operands
    {
        if (symbolTable[i].externalName == operand1)
        {
            inName1 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName1.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    for (unsigned i = 0; i < symbolTable.size(); i++) // get internal names of operands
    {
        if (symbolTable[i].externalName == operand2)
        {
            inName2 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName2.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    
    for (unsigned i = 0; i < symbolTable.size(); i++) // find out which type op1 is
    {
        if (symbolTable[i].externalName == operand1)
        {
            op1Type = symbolTable[i].dataType;
            break;
        } 
    }
    
    for (unsigned i = 0; i < symbolTable.size(); i++) // make sure op2 is the same type
    {
        if (symbolTable[i].externalName == operand2 && symbolTable[i].dataType != op1Type)
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 <<": incompatible types";
            CreateListingTrailer();
            exit(1);
        } 
    }
    
    for (unsigned i = 0; i < symbolTable.size(); i++) // make sure op2 is a var
    {
        if (symbolTable[i].externalName == operand2 && symbolTable[i].mode != VARIABLE)
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 <<": identifier to the left of \":=\" ";
            listingFile << "must be a variable";
            CreateListingTrailer();
            exit(1);
        } 
    }
    
    if (operand1 == operand2)
    {
        return;
    }
    
    if (contentsOfAReg != operand1)
    {
        objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << left << inName1;
        objectFile << setw(5) << "" << endl;
        contentsOfAReg = operand2;
    }
    
    string inName;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == operand1)
        {
            inName = symbolTable[i].internalName;
            break;
        }
    }
    
    objectFile << setw(4) << "" << setw(2) << "" << "STA " << setw(4) << left << inName2;
    objectFile << setw(5) << "" << operand2 << " := " << operand1 << endl;
    
    if (operand1[0] == 'T' && operand1 != "TRUE") // if the operand is a temp, free it
    {
        FreeTemp();
    }
    
    contentsOfAReg = operand2;   
}

void EmitNegationCode(string operand1)
{
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == operand1 && symbolTable[i].dataType != INTEGER)
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 << ": illegal type";
            CreateListingTrailer();
            exit(1);
        }
    }
    
    if (contentsOfAReg[0] == 'T')
    { 
        string inName3;
        for (unsigned i = 0; i < symbolTable.size(); i++)
        {
            if (contentsOfAReg == symbolTable[i].externalName)
            {
                inName3 = symbolTable[i].internalName;
            }
        }
                                                                                        
        objectFile << setw(4) << "" << setw(2) << "" << "STA " << setw(4) << inName3;   
        objectFile << setw(5) << "" << "deassign AReg" << endl;                         
        
        for (unsigned i = 0; i < symbolTable.size(); i++)
        {
            if (symbolTable[i].externalName ==  contentsOfAReg)
            {
                symbolTable[i].alloc = YES;
                break;
            }
        }
        
        contentsOfAReg = "";
    }
    
    string inName1;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand1 == symbolTable[i].externalName)
        {
            inName1 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName1.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    
    objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << "ZERO     " << endl;
    
    bool isDefined = false;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == "ZERO")
        {
            isDefined = true;
            break;
        }
    }
    if (!isDefined)
    {
        Insert("ZERO", INTEGER, CONSTANT, "0", YES, 1);
    }
    
    objectFile << setw(4) << "" << setw(2) << "" << "ISB " << setw(4) << inName1 << setw(5) << "" << "-" << operand1 << endl;
    if (operand1[0] == 'T') // if either operand is a temp, free it
    {
        FreeTemp();
    }
    
    contentsOfAReg = getTemp();
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == contentsOfAReg)
        {
            symbolTable[i].dataType = INTEGER;
            break;
        }
    }
    PushOperand(contentsOfAReg);
}

void EmitNotCode(string operand1)
{
    string inName;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == operand1)
        {
            if (symbolTable[i].dataType != BOOLEAN)
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 << ": boolean expected after \"not\"";
                CreateListingTrailer();
                exit(1);
            }
            inName = symbolTable[i].internalName;
            break;
        }
    }
    if (inName.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    if (contentsOfAReg != operand1)
    {
        objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(5) << inName;
        objectFile << setw(4) << "" << endl;
    }
    
    string label = getLabel();
    objectFile << setw(4) << "" << setw(2) << "" << "AZJ " << setw(4) << left << label;
    objectFile << setw(4) << "" << " not " << operand1 << endl;
    objectFile << setw(4) << "" << setw(2) << "" << "LDA FALS     " << endl;
    bool isDefined = false;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == "FALSE")
        {
            isDefined = true;
            break;
        }
    }
    if (!isDefined)
    {
        Insert("FALS", BOOLEAN, CONSTANT, "0", YES, 1);
    }
    
    objectFile << setw(4) << "" << setw(2) << "" << "UNJ " << setw(4) << label << "+1   " << endl;
    objectFile << setw(4) << left << label;
    objectFile << setw(2) << "" << "LDA TRUE     " << endl;
    isDefined = false;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == "TRUE")
        {
            isDefined = true;
            break;
        }
    }
    if (!isDefined)
    {
        Insert("TRUE", BOOLEAN, CONSTANT, "1", YES, 1);
    }
    
    if (contentsOfAReg[0] == 'T')
    {
        FreeTemp();
    }
    
    contentsOfAReg = getTemp();
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == contentsOfAReg)
        {
            symbolTable[i].dataType = BOOLEAN;
            break;
        }
    }
    PushOperand(contentsOfAReg);
}

void EmitEqualsCode(string operand1, string operand2)
{
    storeType op1;
    string inName1, inName2;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == operand1)
        {
            op1 = symbolTable[i].dataType;
            break;
        }
    }
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == operand2 && symbolTable[i].dataType != op1)
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 << ": incompatible types";
            CreateListingTrailer();
            exit(1);
        }
    }
    
    if (contentsOfAReg[0] == 'T')
    {
        if (contentsOfAReg != operand1 && contentsOfAReg != operand2) //if a reg holds a temp not
        {                         
            string inName3;
            for (unsigned i = 0; i < symbolTable.size(); i++)
            {
                if (contentsOfAReg == symbolTable[i].externalName)
                {
                    inName3 = symbolTable[i].internalName;
                }
            }
                                                                                                    //operand 1 or 2, deassign it,
            objectFile << setw(4) << "" << setw(2) << "" << "STA " << setw(4) << left << inName3;    //store the temp into memory,
            objectFile << setw(5) << "" << "deassign AReg" << endl;                                //and change the alloc entry in the table to yes
            
            for (unsigned i = 0; i < symbolTable.size(); i++)
            {
                if (symbolTable[i].externalName ==  contentsOfAReg)
                {
                    symbolTable[i].alloc = YES;
                    break;
                }
            }
            
            contentsOfAReg = "";
        }
    }
    
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand2 == symbolTable[i].externalName)
        {
            inName2 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName2.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand1 == symbolTable[i].externalName)
        {
            inName1 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName1.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    
    if (contentsOfAReg != operand1 && contentsOfAReg != operand2)
    {
        objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(5) << left << inName2;
        objectFile << setw(4) << "" << endl;
    }
    
    if (contentsOfAReg == operand1)
    {
        objectFile << setw(4) << "" << setw(2) << "" << "ISB " << setw(4) << left << inName2 << setw(5) << "";
    }
    else 
    {
        objectFile << setw(4) << "" << setw(2) << "" << "ISB " << setw(4) << left << inName1 << setw(5) << "";
    }
    objectFile << operand2 << " = " << operand1 << endl;
    string label = getLabel();
    objectFile << setw(4) << "" << setw(2) << "" << "AZJ " << setw(4) << label << setw(5) << "" << endl;
    objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << "FALS     " << endl;
    
    bool isDefined = false;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == "FALSE")
        {
            isDefined = true;
            break;
        }
    }
    if (!isDefined)
    {
        Insert("FALS", BOOLEAN, CONSTANT, "0", YES, 1);
    }
    
    objectFile << setw(4) << "" << setw(2) << "" << "UNJ " << setw(4) << label << "+1   " << endl;
    objectFile << setw(4) << label << setw(2) << "" << "LDA " << setw(4) << "TRUE" << setw(5) << "" << endl;
    
    isDefined = false;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == "TRUE")
        {
            isDefined = true;
            break;
        }
    }
    if (!isDefined)
    {
        Insert("TRUE", BOOLEAN, CONSTANT, "1", YES, 1);
    }
    
    if (operand1[0] == 'T') // if either operand is a temp, free it
    {
        FreeTemp();
    }
    if (operand2[0] == 'T')
    {
        FreeTemp();
    }
    
    contentsOfAReg = getTemp();
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == contentsOfAReg)
        {
            symbolTable[i].dataType = BOOLEAN;
            break;
        }
    }
    PushOperand(contentsOfAReg);
}

void EmitDoesNotEqualCode(string operand1, string operand2)
{
    storeType op1;
    string inName1, inName2;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == operand1)
        {
            op1 = symbolTable[i].dataType;
            break;
        }
    }
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == operand2 && symbolTable[i].dataType != op1)
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 << ": incompatible types";
            CreateListingTrailer();
            exit(1);
        }
    }
    
    if (contentsOfAReg[0] == 'T')
    {
        if (contentsOfAReg != operand1 && contentsOfAReg != operand2) //if a reg holds a temp not
        {                         
            string inName3;
            for (unsigned i = 0; i < symbolTable.size(); i++)
            {
                if (contentsOfAReg == symbolTable[i].externalName)
                {
                    inName3 = symbolTable[i].internalName;
                }
            }
                                                                                                    //operand 1 or 2, deassign it,
            objectFile << setw(4) << "" << setw(2) << "" << "STA " << setw(4) << inName3;    //store the temp into memory,
            objectFile << setw(5) << "" << "deassign AReg" << endl;                                //and change the alloc entry in the table to yes
            
            for (unsigned i = 0; i < symbolTable.size(); i++)
            {
                if (symbolTable[i].externalName ==  contentsOfAReg)
                {
                    symbolTable[i].alloc = YES;
                    break;
                }
            }
            
            contentsOfAReg = "";
        }
    }
    
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand2 == symbolTable[i].externalName)
        {
            inName2 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName2.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand1 == symbolTable[i].externalName)
        {
            inName1 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName1.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    
    if (contentsOfAReg != operand1 && contentsOfAReg != operand2)
    {
        objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(5) << left << inName2;
        objectFile << setw(4) << "" << endl;
    }
    
    if (contentsOfAReg == operand1)
    {
        objectFile << setw(4) << "" << setw(2) << "" << "ISB " << setw(4) << left << inName2 << setw(5) << "";
    }
    else 
    {
        objectFile << setw(4) << "" << setw(2) << "" << "ISB " << setw(4) << left << inName1 << setw(5) << "";
    }
    objectFile << operand2 << " <> " << operand1 << endl;
    string label = getLabel();
    objectFile << setw(4) << "" << setw(2) << "" << "AZJ " << setw(4) << label << setw(5) << "+1" << endl;
    objectFile << setw(4) << label << "" << setw(2) << "" << "LDA " << setw(4) << "TRUE     " << endl;
    
    bool isDefined = false;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == "TRUE")
        {
            isDefined = true;
            break;
        }
    }
    if (!isDefined)
    {
        Insert("TRUE", BOOLEAN, CONSTANT, "1", YES, 1);
    }
    
    if (operand1[0] == 'T') // if either operand is a temp, free it
    {
        FreeTemp();
    }
    if (operand2[0] == 'T')
    {
        FreeTemp();
    }
    
    contentsOfAReg = getTemp();
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == contentsOfAReg)
        {
            symbolTable[i].dataType = BOOLEAN;
            break;
        }
    }
    PushOperand(contentsOfAReg);
}

void EmitLessThanCode(string operand1, string operand2)
{
    string inName1, inName2;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand2 == symbolTable[i].externalName)
        {
            inName2 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName2.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand1 == symbolTable[i].externalName)
        {
            inName1 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName1.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    
    for (unsigned i = 0; i < symbolTable.size(); i++) // make sure each operand is an integer
    {
        if (symbolTable[i].externalName == operand1 || symbolTable[i].externalName == operand2)
        {
            if (symbolTable[i].dataType != INTEGER) // if it isnt an integer, process error
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 << ": illegal type";
                CreateListingTrailer();
                exit(1);
            }
        } 
    }
    if (contentsOfAReg[0] == 'T' && contentsOfAReg != operand2)                                 //if a reg holds a temp not
    {                                                                                           //operand 2, deassign it,
        objectFile << setw(4) << "" << setw(2) << "" << "STA " << setw(4) << left << contentsOfAReg;    //store the temp into memory,
        objectFile << setw(5) << "" << "deassign AReg" << endl;                                //and change the alloc entry in the table to yes
        
        for (unsigned i = 0; i < symbolTable.size(); i++)
        {
            if (symbolTable[i].externalName ==  contentsOfAReg)
            {
                symbolTable[i].alloc = YES;
                break;
            }
        }
        
        contentsOfAReg = "";
    }
    
    if (contentsOfAReg != operand2)
    {
        contentsOfAReg = "";
        objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << left << inName2;
        objectFile << setw(5) << "" << endl;
    }
        
    objectFile << setw(4) << "" << setw(2) << "" << "ISB " << setw(4) << left << inName1; // emit code to perform subtraction
    objectFile << setw(5) << "" << operand2 << " < " << operand1 << endl;
    string label = getLabel();
    objectFile << setw(4) << "" << setw(2) << "" << "AMJ " << setw(4) << left << label << setw(5) << "" << endl;
    objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << "FALS     " << endl;
    
    bool isDefined = false;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == "FALSE")
        {
            isDefined = true;
            break;
        }
    }
    if (!isDefined)
    {
        Insert("FALS", BOOLEAN, CONSTANT, "0", YES, 1);
    }
    objectFile << setw(4) << "" << setw(2) << "" << "UNJ " << setw(4) << label << "+1   " << endl;
    objectFile << setw(4) << label << setw(2) << "" << "LDA " << setw(4) << "TRUE     " << endl;
    
    isDefined = false;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == "TRUE")
        {
            isDefined = true;
            break;
        }
    }
    if (!isDefined)
    {
        Insert("TRUE", BOOLEAN, CONSTANT, "1", YES, 1);
    }
    
    contentsOfAReg = getTemp();
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == contentsOfAReg)
        {
            symbolTable[i].dataType = BOOLEAN;
            break;
        }
    }
    PushOperand(contentsOfAReg);
}

void EmitLessThanOrEqualsCode(string operand1, string operand2)
{
    string inName1, inName2;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand2 == symbolTable[i].externalName)
        {
            inName2 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName2.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand1 == symbolTable[i].externalName)
        {
            inName1 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName1.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    
    for (unsigned i = 0; i < symbolTable.size(); i++) // make sure each operand is an integer
    {
        if (symbolTable[i].externalName == operand1 || symbolTable[i].externalName == operand2)
        {
            if (symbolTable[i].dataType != INTEGER) // if it isnt an integer, process error
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 << ": illegal type" << endl;
                CreateListingTrailer();
                exit(1);
            }
        } 
    }
    if (contentsOfAReg[0] == 'T' && contentsOfAReg != operand2)                                 //if a reg holds a temp not
    {                                                                                           //operand 2, deassign it,
        objectFile << setw(4) << "" << setw(2) << "" << "STA " << setw(4) << contentsOfAReg;    //store the temp into memory,
        objectFile << setw(5) << "" << "deassign AReg" << endl;                                //and change the alloc entry in the table to yes
        
        for (unsigned i = 0; i < symbolTable.size(); i++)
        {
            if (symbolTable[i].externalName ==  contentsOfAReg)
            {
                symbolTable[i].alloc = YES;
                break;
            }
        }
        
        contentsOfAReg = "";
    }
    
    if (contentsOfAReg != operand2)
    {
        contentsOfAReg = "";
        objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << inName2;
        objectFile << setw(5) << "" << endl;
    }
        
    objectFile << setw(4) << "" << setw(2) << "" << "ISB " << setw(4) << inName1; // emit code to perform subtraction
    objectFile << setw(5) << "" << operand2 << " <= " << operand1 << endl;
    string label = getLabel();    
    objectFile << setw(4) << "" << setw(2) << "" << "AMJ " << setw(4) << label << setw(5) << "" << endl;
    objectFile << setw(4) << "" << setw(2) << "" << "AZJ " << setw(4) << label << setw(5) << "" << endl;
    objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << "FALS     " << endl;
    
    bool isDefined = false;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == "FALSE")
        {
            isDefined = true;
            break;
        }
    }
    if (!isDefined)
    {
        Insert("FALS", BOOLEAN, CONSTANT, "0", YES, 1);
    }
    objectFile << setw(4) << "" << setw(2) << "" << "UNJ " << setw(4) << label << "+1   " << endl;
    objectFile << setw(4) << label << setw(2) << "" << "LDA " << setw(4) << "TRUE     " << endl;
    
    contentsOfAReg = getTemp();
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == contentsOfAReg)
        {
            symbolTable[i].dataType = BOOLEAN;
            break;
        }
    }
    PushOperand(contentsOfAReg);
}

void EmitGreaterThanCode(string operand1, string operand2)
{
    string inName1, inName2;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand2 == symbolTable[i].externalName)
        {
            inName2 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName2.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand1 == symbolTable[i].externalName)
        {
            inName1 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName1.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    
    for (unsigned i = 0; i < symbolTable.size(); i++) // make sure each operand is an integer
    {
        if (symbolTable[i].externalName == operand1 || symbolTable[i].externalName == operand2)
        {
            if (symbolTable[i].dataType != INTEGER) // if it isnt an integer, process error
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 << ": illegal type";
                CreateListingTrailer();
                exit(1);
            }
        } 
    }
    if (contentsOfAReg[0] == 'T' && contentsOfAReg != operand2)                                 //if a reg holds a temp not
    {                                                                                           //operand 2, deassign it,
        objectFile << setw(4) << "" << setw(2) << "" << "STA " << setw(4) << contentsOfAReg;    //store the temp into memory,
        objectFile << setw(5) << "" << "deassign AReg" << endl;                                //and change the alloc entry in the table to yes
        
        for (unsigned i = 0; i < symbolTable.size(); i++)
        {
            if (symbolTable[i].externalName ==  contentsOfAReg)
            {
                symbolTable[i].alloc = YES;
                break;
            }
        }
        
        contentsOfAReg = "";
    }
    
    if (contentsOfAReg != operand2)
    {
        contentsOfAReg = "";
        objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << inName2;
        objectFile << setw(5) << "" << endl;
    }
        
    objectFile << setw(4) << "" << setw(2) << "" << "ISB " << setw(4) << inName1; // emit code to perform subtraction
    objectFile << setw(5) << "" << operand2 << " > " << operand1 << endl;
    string label = getLabel();    
    objectFile << setw(4) << "" << setw(2) << "" << "AMJ " << setw(4) << label << setw(5) << "" << endl;
    objectFile << setw(4) << "" << setw(2) << "" << "AZJ " << setw(4) << label << setw(5) << "" << endl;
    objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << "TRUE     " << endl;
    
    bool isDefined = false;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == "TRUE")
        {
            isDefined = true;
            break;
        }
    }
    if (!isDefined)
    {
        Insert("TRUE", BOOLEAN, CONSTANT, "1", YES, 1);
    }
    
    isDefined = false;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == "FALSE")
        {
            isDefined = true;
            break;
        }
    }
    if (!isDefined)
    {
        Insert("FALS", BOOLEAN, CONSTANT, "0", YES, 1);
    }
    objectFile << setw(4) << "" << setw(2) << "" << "UNJ " << setw(4) << label << "+1   " << endl;
    objectFile << setw(4) << label << setw(2) << "" << "LDA " << setw(4) << "FALS     " << endl;
    
    contentsOfAReg = getTemp();
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == contentsOfAReg)
        {
            symbolTable[i].dataType = BOOLEAN;
            break;
        }
    }
    PushOperand(contentsOfAReg);
}

void EmitGreaterThanOrEqualsCode(string operand1, string operand2)
{
    string inName1, inName2;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand2 == symbolTable[i].externalName)
        {
            inName2 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName2.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (operand1 == symbolTable[i].externalName)
        {
            inName1 = symbolTable[i].internalName;
            break;
        }
    }
    if (inName1.empty())
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 << ": reference to undefined variable";
        CreateListingTrailer();
        exit(1);
    }
    
    for (unsigned i = 0; i < symbolTable.size(); i++) // make sure each operand is an integer
    {
        if (symbolTable[i].externalName == operand1 || symbolTable[i].externalName == operand2)
        {
            if (symbolTable[i].dataType != INTEGER) // if it isnt an integer, process error
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 << ": illegal type";
                CreateListingTrailer();
                exit(1);
            }
        } 
    }
    if (contentsOfAReg[0] == 'T' && contentsOfAReg != operand2)                                 //if a reg holds a temp not
    {                                                                                           //operand 2, deassign it,
        objectFile << setw(4) << "" << setw(2) << "" << "STA " << setw(4) << contentsOfAReg;    //store the temp into memory,
        objectFile << setw(5) << "" << "deassign AReg" << endl;                                //and change the alloc entry in the table to yes
        
        for (unsigned i = 0; i < symbolTable.size(); i++)
        {
            if (symbolTable[i].externalName ==  contentsOfAReg)
            {
                symbolTable[i].alloc = YES;
                break;
            }
        }
        
        contentsOfAReg = "";
    }
    
    if (contentsOfAReg != operand2)
    {
        contentsOfAReg = "";
        objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << inName2;
        objectFile << setw(5) << "" << endl;
    }
        
    objectFile << setw(4) << "" << setw(2) << "" << "ISB " << setw(4) << inName1; // emit code to perform subtraction
    objectFile << setw(5) << "" << operand2 << " >= " << operand1 << endl;
    string label = getLabel();    
    objectFile << setw(4) << "" << setw(2) << "" << "AMJ " << setw(4) << label << setw(5) << "" << endl;
    objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << "TRUE     " << endl;
    
    bool isDefined = false;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == "TRUE")
        {
            isDefined = true;
            break;
        }
    }
    if (!isDefined)
    {
        Insert("TRUE", BOOLEAN, CONSTANT, "1", YES, 1);
    }
    
    isDefined = false;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == "FALSE")
        {
            isDefined = true;
            break;
        }
    }
    if (!isDefined)
    {
        Insert("FALS", BOOLEAN, CONSTANT, "0", YES, 1);
    }
    objectFile << setw(4) << "" << setw(2) << "" << "UNJ " << setw(4) << label << "+1   " << endl;
    objectFile << setw(4) << label << setw(2) << "" << "LDA " << setw(4) << "FALS     " << endl;
    
    contentsOfAReg = getTemp();
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == contentsOfAReg)
        {
            symbolTable[i].dataType = BOOLEAN;
            break;
        }
    }
    PushOperand(contentsOfAReg);
}

void EmitThenCode(string operand) //emit code that follows 'then' and statement predicate
{
    string tempLabel, inName;
    tempLabel = getLabel();
    
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == operand)
        {
            if (symbolTable[i].dataType != BOOLEAN)
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 << ": predicate following \"if\" must be boolean valued";
                CreateListingTrailer();
                exit(1);
            }
            else
            {
                inName = symbolTable[i].internalName;
            }
        }
    }
    
    if (contentsOfAReg != operand)
    {
        objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << left << inName << setw(5) << "" << endl;
    }
    
    objectFile << setw(4) << "" << setw(2) << "" << "AZJ " << setw(4) << left << tempLabel << setw(5) << "";
    objectFile << "if false jump to " << tempLabel << endl;
    PushOperand(tempLabel);
    if (operand[0] == 'T')
    {
        FreeTemp();
    }
    contentsOfAReg = "";
}

void EmitElseCode(string operand)
{
    string tempLabel;
    tempLabel = getLabel();
    objectFile << setw(4) << "" << setw(2) << "" << "UNJ " << setw(4) << left << tempLabel << setw(5) << "" << "jump to end if" << endl;;
    objectFile << setw(4) << left << operand << setw(2) << "" << "NOP " << setw(4) << "" << setw(5) << "" << "else" << endl;
    PushOperand(tempLabel);
    contentsOfAReg = "";
}

void EmitPostIfCode(string operand)
{
    //emit instruction to label this point of object code with the argument operand
    objectFile << setw(4) << left << operand << setw(2) << "" << "NOP " << setw(4) << "" << setw(5) << "" << "end if" << endl;
    
    //deassign operands from all registers
    contentsOfAReg = "";
}

void EmitWhileCode()
{
    string tempLabel;
    tempLabel = getLabel();
    objectFile << setw(4) << left << tempLabel << setw(2) << "" << "NOP " << setw(4) << "" << setw(5) << "" << "while" << endl;
    PushOperand(tempLabel);
    contentsOfAReg = "";
}

void EmitDoCode(string operand)
{
    string tempLabel, inName;
    tempLabel = getLabel();
    
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == operand)
        {
            if (symbolTable[i].dataType != BOOLEAN)
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 << ": predicate following \"while\" must be boolean valued";
                CreateListingTrailer();
                exit(1);
            }
            else
            {
                inName = symbolTable[i].internalName;
            }
        }
    }
    
    if (contentsOfAReg != operand)
    {
        objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << left << inName << setw(5) << "" << endl;
    }
    
    objectFile << setw(4) << "" << setw(2) << "" << "AZJ " << setw(4) << left << tempLabel << setw(5) << "" << "do" << endl;
    PushOperand(tempLabel);
    if (operand[0] == 'T')
    {
        FreeTemp();
    }
    contentsOfAReg = "";
}

void EmitPostWhileCode(string operand1, string operand2)
// operand 2 is the label at the beginning of the loop
// operand 1 is the label that should follow the loop
{
    objectFile << setw(4) << "" << setw(2) << "" << "UNJ " << setw(4) << left << operand2 << setw(5) << "" << "end while" << endl;
    objectFile << setw(4) << left << operand1 << setw(2) << "" << "NOP " << setw(4) << "" << setw(5) << "" << endl;
    contentsOfAReg = "";
}

void EmitRepeatCode()
{
    string tempLabel;
    tempLabel = getLabel();
    objectFile << setw(4) << left << tempLabel << setw(2) << "" << "NOP " << setw(4) << "" << setw(5) << "" << "repeat" << endl;
    PushOperand(tempLabel);
    contentsOfAReg = "";
}

void EmitUntilCode(string operand1, string operand2)
//emit code that follows until and the predicate of loop. operand1 is the value of the predicate.  
//operand2 is the label that points to the beginning of the loop
{
    string inName;
    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if (symbolTable[i].externalName == operand1)
        {
            if (symbolTable[i].dataType != BOOLEAN)
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 << ": predicate following \"until\" must be boolean valued";
                CreateListingTrailer();
                exit(1);
            }
            else
            {
                inName = symbolTable[i].internalName;
            }
        }
    }
    
    if (contentsOfAReg != operand1)
    {
        objectFile << setw(4) << "" << setw(2) << "" << "LDA " << setw(4) << left << inName << setw(5) << "" << endl;
    }
    
    objectFile << setw(4) << "" << setw(2) << "" << "AZJ " << setw(4) << left << operand2 << setw(5) << "" << "until" << endl;
    if (operand1[0] == 'T')
    {
        FreeTemp();
    }
    contentsOfAReg = "";
}

void ConstStmts() //token should now be NON_KEY_ID
{
    string x, y;
    if (isKeyword(token)) //make sure token is not a keyword
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": non-keyword identifier expected";
        CreateListingTrailer();
        exit(1);
    }

    x = token; //name of const
    if (NextToken() != "=") // make sure non key id is followed by an equals sign
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": \"=\" expected";
        CreateListingTrailer();
        exit(1);
    }

    y = NextToken(); //value of x    

    if (y == "+" || y == "-") //if token begins with a sign, make sure all characters following the sign are digits
    {
        NextToken();
        string z = token;

        stringstream is(z);
        int i;

        is >> i;
        if (i)
        {
            y = y + token;
        }
        else 
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 <<": integer expected after sign";
            CreateListingTrailer();
            exit(1);
        }

    }

    if (y == "not") //if token is not, make sure it is followed by true or false
    {
        string inName;
        NextToken();
        for (unsigned i = 0; i < symbolTable.size(); i++)
        {
            if (token == symbolTable[i].externalName)
            {
                if (symbolTable[i].dataType != BOOLEAN)
                {
                    numErrors += 1;
                    listingFile << endl << "Error: Line " << lineNumber - 1 <<": boolean expected after not";
                    CreateListingTrailer();
                    exit(1);
                }
                
                if (symbolTable[i].value == "0")
                {
                    Insert(x.substr(0, 15), BOOLEAN, CONSTANT, "1", YES, 1); //insert value into symbol table
                }
                else
                {
                    Insert(x.substr(0, 15), BOOLEAN, CONSTANT, "0", YES, 1); //insert value into symbol table
                }
                
                if (NextToken() != ";") //make sure each literal is followed by a ";"
                {
                    numErrors += 1;
                    listingFile << endl << "Error: Line " << lineNumber - 1 <<": \";\" expected";
                    CreateListingTrailer();
                    exit(1);
                }
                if (!isKeyword(NextToken())) //if next token is non key id, run conststmts again
                    ConstStmts();
                    
                if (token != "var" && token != "begin") // if it is a keyword, but is not begin or var, exit
                {
                    numErrors += 1;
                    listingFile << endl << "Error: Line " << lineNumber - 1 <<": non-keyword identifier,\"begin\", or \"var\" expected";
                    CreateListingTrailer();
                    exit(1);
                }
                return;
            }
        }
        
        if (token != "true" && token != "false")
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 <<": boolean expected after not";
            CreateListingTrailer();
            exit(1);
        }

        if (token == "true") 
            y = "false";
        else
            y = "true";
    }

    bool isDefined = false;

    for (unsigned i = 0; i < symbolTable.size(); i++) //check if token has already been defined
    {
        if (y.substr(0,15) == symbolTable[i].externalName)
        {
            isDefined = true;
        }
    }
    
    if (!isDefined)
    {
        if (!isKeyword(y) && y.find_first_not_of("+-0123456789") != string::npos)
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 <<": reference to undefined constant";
            CreateListingTrailer();
            exit(1);
        }
    }

    if (NextToken() != ";") //make sure each literal is followed by a ";"
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": \";\" expected";
        CreateListingTrailer();
        exit(1);
    }
    
    Insert(x.substr(0, 15), WhichType(y), CONSTANT, WhichValue(y), YES, 1); //insert value into symbol table

    if (!isKeyword(NextToken())) //if next token is non key id, run conststmts again
        ConstStmts();
        
    if (token != "var" && token != "begin") // if it is a keyword, but is not begin or var, exit
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": non-keyword identifier,\"begin\", or \"var\" expected";
        CreateListingTrailer();
        exit(1);
    }

    
}

void VarStmts() // token should be NON_KEY_ID
{
    string x,y;
    if (isKeyword(token)) //make sure token is not a reserved keyword
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": non-keyword identifier expected";
        CreateListingTrailer();
        exit(1);
    }

    x = Ids(); //gets all var names if it is a list

    if (token != ":") // make sure token following non-key ids is a colon
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": \":\" expected";
        CreateListingTrailer();
        exit(1);
    }

    if (NextToken() != "integer" && token != "boolean") //token following colon must be a data type
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": illegal type follows \":\"";
        CreateListingTrailer();
        exit(1);
    }

    y = token; //save data type

    if(NextToken() != ";") //make sure data type is followed by semicolon
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": semicolon expected";
        CreateListingTrailer();
        exit(1);
    }

    if (y == "integer") //insert integer values into table
    {
        Insert(x, INTEGER, VARIABLE, "", YES, 1);
    }
    else if (y == "boolean") //insert boolean values into table
    {
        Insert(x, BOOLEAN, VARIABLE, "", YES, 1);
    }

    if (!isKeyword(NextToken())) //if followed by non key id, run varstmts again
    {
        VarStmts();
    }
    if (token != "begin") //if it is a keyword, make sure it is "begin"
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": non-keyword identifier or \"begin\" expected";
        CreateListingTrailer();
        exit(1);
    }
    
}

string Ids() //puts all non key ids from a list into one string to be inserted into table
{
    string temp, tempString;
    if (isKeyword(token)) //make sure token is not reserved keyword
    {
        numErrors += 1;
        listingFile << endl << "Error: Line " << lineNumber - 1 <<": non-keyword identifier expected";
        CreateListingTrailer();
        exit(1);
    }

    tempString = token;
    temp = token;

    if (NextToken() == ",") //look for commas separating non key ids
    {
        if(isKeyword(NextToken())) //make sure following token isnt a keyword
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 <<": non-keyword identifier expected";
            CreateListingTrailer();
            exit(1);
        }
        tempString = temp + "," + Ids(); //attach non key id to string
    }
    return tempString;
}

void Code(string oprtr, string operand1, string operand2)
{
    if (oprtr == "program")
    {
        objectFile << "STRT  NOP" << setw(10) << "" << symbolTable[0].externalName << " - Reese Montgomery - Stage 2" << endl;
    }
    else if (oprtr == "end" && beginNo == 1)
    {
        PrintSymbolTable();
    }
    else if (oprtr == "end" && beginNo > 1)
    {
        if (NextToken() != ";")
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 <<": \";\" expected after \"end\"";
            CreateListingTrailer();
            exit(1);
        }
        beginNo--;
    }
    else if (oprtr == "read")
    {
        ReadStmt();
    }
    else if (oprtr == "write")
    {
        WriteStmt();
    }
    else if (oprtr == "+")
    {
        EmitAdditionCode(operand1, operand2);
    }
    else if (oprtr == "-")
    {
        EmitSubtractionCode(operand1, operand2);
    }
    else if (oprtr == "neg")
    {
        EmitNegationCode(operand1);
    }
    else if (oprtr == "not")
    {
        EmitNotCode(operand1);
    }
    else if (oprtr == "*")
    {
        EmitMultiplicationCode(operand1, operand2);
    }
    else if (oprtr == "div")
    {
        EmitDivisionCode(operand1, operand2);
    }
    else if (oprtr == "mod")
    {
        EmitModuloCode(operand1, operand2);
    }
    else if (oprtr == "and")
    {
        EmitAndCode(operand1, operand2);
    }
    else if (oprtr == "or")
    {
        EmitOrCode(operand1, operand2);
    }
    else if (oprtr == "=")
    {
        EmitEqualsCode(operand1, operand2);
    }
    else if (oprtr == ":=")
    {
        EmitAssignCode(operand1, operand2);
    }
    else if (oprtr == "<")
    {
        EmitLessThanCode(operand1, operand2);
    }
    else if (oprtr == "<=")
    {
        EmitLessThanOrEqualsCode(operand1, operand2);
    }
    else if (oprtr == ">")
    {
        EmitGreaterThanCode(operand1, operand2);
    }
    else if (oprtr == ">=")
    {
        EmitGreaterThanOrEqualsCode(operand1, operand2);
    }
    else if (oprtr == "<>")
    {
        EmitDoesNotEqualCode(operand1, operand2);
    }
    else if (oprtr == "then")
    {
        EmitThenCode(operand1);
    }
    else if (oprtr == "else")
    {
        EmitElseCode(operand1);
    }
    else if (oprtr == "post_if")
    {
        EmitPostIfCode(operand1);
    }
    else if (oprtr == "do")
    {
        EmitDoCode(operand1);
    }
    else if (oprtr == "while")
    {
        EmitWhileCode();
    }
    else if (oprtr == "post_while")
    {
        EmitPostWhileCode(operand1, operand2);
    }
    else if (oprtr == "repeat")
    {
        EmitRepeatCode();
    }
    else if (oprtr == "until")
    {
        EmitUntilCode(operand1, operand2);
    }
    else 
    {
        numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 << " undefined operation";
                CreateListingTrailer();
                exit(1);
    }
}

void Insert(string externalName, storeType inType, modes inMode, string inValue,
            allocation inAlloc, int inUnits)
    //create symbol table entry for each identifier in list of external names
    //multiply inserted names are illegal
{
    if (externalName == "FALS")
    {
        entry newEntry = {externalName, "FALSE" , inType, inMode, inValue.substr(0,15), inAlloc, inUnits};
        if (symbolTable.size() == MAX_SYMBOL_TABLE_SIZE)
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 <<": Symbol table overflow";
            CreateListingTrailer();
            exit(1);
        }
        symbolTable.push_back(newEntry);
        return;
    }
    if (externalName == "ZERO")
    {
        entry newEntry = {externalName, externalName , inType, inMode, inValue.substr(0,15), inAlloc, inUnits};
        if (symbolTable.size() == MAX_SYMBOL_TABLE_SIZE)
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 <<": Symbol table overflow";
            CreateListingTrailer();
            exit(1);
        }
        symbolTable.push_back(newEntry);
        return;
    }
    
    string name;

    while (externalName.length() != 0) //this while loop extracts each non key id from a string delimited by commas
    {
        name = "";
        for (unsigned i = 0; i < externalName.length(); i++)
        {
            if (externalName[i] == ',') // look for comma in string, if found, break out of for loop
            {
                break;
            }
            name += externalName[i];
        }

        if(name.length() == externalName.length()) // if name length == external name length, we've reached the end of the list
        {
            externalName = "";
        }
        else
        {
            externalName = externalName.substr(name.length() + 1, externalName.length() - name.length()); //if not, get rid of the part of external name we just copied into name
        }

        for (unsigned i = 0; i < symbolTable.size(); i++) //loop through all current entries of symbol table
        {
            if(symbolTable[i].externalName == name.substr(0, 15)) //check if name has alread been defined in symbolTable
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 <<": " << name.substr(0,15) << " is multiply defined";
                CreateListingTrailer();
                exit(1);
            } 
        }

        if(isKeyword(name) && name != "true" && name != "false") //check if name is a reserved keyword
        {
            numErrors += 1;
            listingFile << endl << "Error: Line " << lineNumber - 1 <<": illegal use of keyword";
            CreateListingTrailer();
            exit(1);
        }
        else if (inType == UNKNOWN)
        {
            stringstream temp;
            temp << "T" << currentTempNo;
            entry newEntry = {temp.str(), name.substr(0, 15) , inType, inMode, inValue.substr(0,15), inAlloc, inUnits};
            if (symbolTable.size() == MAX_SYMBOL_TABLE_SIZE)
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 <<": Symbol table overflow";
                CreateListingTrailer();
                exit(1);
            }
            symbolTable.push_back(newEntry);
        }
        else //create table entry
        {
            entry newEntry = {GenInternalName(inType), name.substr(0, 15) , inType, inMode, inValue.substr(0,15), inAlloc, inUnits};
            if (symbolTable.size() == MAX_SYMBOL_TABLE_SIZE)
            {
                numErrors += 1;
                listingFile << endl << "Error: Line " << lineNumber - 1 <<": Symbol table overflow";
                CreateListingTrailer();
                exit(1);
            }
            symbolTable.push_back(newEntry);
        }
    }
}

string GenInternalName(storeType inType) // this aptly named function generates the internal name of a symbol for the symbol table
{
    stringstream s;

    if (inType == PROG_NAME)
    {
        return "P0";
    }
    else if (inType == INTEGER)
    {
        numInts += 1;
        s << "I" << numInts;
        return s.str();
    }
    else
    {
        numBools += 1;
        s << "B" << numBools;
        return s.str();
    }
}

storeType WhichType(string name) //this function returns whether the value of the string is a boolean or an integer value
{
    istringstream inputString(name);
    int i;
    string s;

    inputString >> i;
    if (inputString)
    {
        return INTEGER;
    }

    inputString.clear();
    inputString.str(name);

    inputString >> s;
    if (s == "true" || s == "false" || s == "not")
    {
        return BOOLEAN;
    }

    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if(symbolTable[i].externalName == name) //check if name has alread been defined in symbolTable
        {
            return symbolTable[i].dataType;
        } 
    }

    numErrors += 1;
    listingFile << endl << "Error: Line " << lineNumber - 1 <<": reference to undefined constant";
    CreateListingTrailer();
    exit(1);

}

string WhichValue(string name) // this function returns the value of the symbol for the symbol table
{
    if (name == "true" || name == "false")
    {
        return (name == "true") ? "1" : "0"; //if name is true, return 1, if false, return 0
    }

    if (name == "not true" || name == "not false")
    {
        return (name == "not true") ? "0" : "1"; //if name is not true, return 0, if not false, return 1
    }

    istringstream inputString(name);
    int i;

    inputString >> i;
    if (inputString)
    {
        return name;
    }

    for (unsigned i = 0; i < symbolTable.size(); i++)
    {
        if(symbolTable[i].externalName == name) //check if name has alread been defined in symbolTable
        {
            return symbolTable[i].value;
        } 
    }
    
    numErrors += 1;
    listingFile << endl << "Error: Line " << lineNumber - 1 <<": reference to undefined constant";
    CreateListingTrailer();
    exit(1);
}

void FreeTemp()
{
    currentTempNo--;
    if (currentTempNo < -1)
    {
        numErrors += 1;
        listingFile << endl << "Compiler Error: currentTempNo should be >= -1";
        CreateListingTrailer();
        exit(1);
    }
}

string getTemp()
{
    stringstream temp;
    currentTempNo++;
    temp << "T" << currentTempNo;
    if (currentTempNo > maxTempNo)
    {
        Insert(temp.str(), UNKNOWN, VARIABLE, "", NO , 1);
        maxTempNo++;
    }
    
    return temp.str();    
}

string getLabel()
{
    stringstream temp;
    labelNo++;
    temp << "L" << labelNo;
    
    return temp.str();
}

string NextToken() ////returns the next token or end of file marker
{
    token = "";
    while (token == "")
    {
        switch(charac)
        {
            case '{'                : //process comment
                                    while (charac != END_OF_FILE && charac != '}')
                                    {
                                        NextChar();
                                    }
                                    if (charac == END_OF_FILE)
                                    {
                                        numErrors += 1;
                                        listingFile << endl << "Error: Line " << lineNumber - 1 <<": unexpected end of file";
                                        CreateListingTrailer();
                                        exit(1);
                                    }
                                    NextChar();
                                    break;

            case '}'                : numErrors += 1;
                                    listingFile << endl << "Error: Line " << lineNumber - 1 <<": '}' cannot begin token";
                                    CreateListingTrailer();
                                    exit(1);
                                    break;
            
            case ' ' : //ignore whitespace
            case '\n':
            case '\r':
            case '\t':
                                    NextChar();
                                    break;

            case ':': //special characters
                                    token += charac;
                                    if (NextChar() != '=')
                                    {
                                        break;
                                    }
                                    else
                                    {
                                        token += charac;
                                        NextChar();
                                    }
                                    break;
            case '<':
                                    token += charac;
                                    if (NextChar() != '=' && charac != '>')
                                    {
                                        break;
                                    }
                                    else
                                    {
                                        token += charac;
                                        NextChar();
                                    }
                                    break;
            case '>':
                                    token += charac;
                                    if (NextChar() != '=')
                                    {
                                        break;
                                    }
                                    else
                                    {
                                        token += charac;
                                        NextChar();
                                    }
                                    break;
            case ',':
            case ';':
            case '=':
            case '+':
            case '-':
            case '.':
            case '(':
            case ')':
            case '*':
                                    token += charac;
                                    NextChar();
                                    break;

            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
            case 'g':
            case 'h':
            case 'i':
            case 'j':
            case 'k':
            case 'l':
            case 'm': 
            case 'n': 
            case 'o': 
            case 'p':               
            case 'q': 
            case 'r': 
            case 's': 
            case 't': 
            case 'u': 
            case 'v': 
            case 'w': 
            case 'x': 
            case 'y': 
            case 'z': 
                                    token = charac;
                                    while (isalnum(NextChar()) || charac == '_')
                                    {
                                        token += charac;
                                    }
                                    if (token[token.length() - 1] == '_')
                                    {
                                        numErrors += 1;
                                        listingFile << endl << "Error: Line " << lineNumber - 1 <<": '_' cannot end token";
                                        CreateListingTrailer();
                                        exit(1);
                                    }
                                    break;

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                                    token = charac;
                                    while (isdigit(NextChar())) token += charac;
                                    break;

            case END_OF_FILE        : token = charac;
                                    break;

            default                 : numErrors += 1;
                                    listingFile << endl << "Error: Line " << lineNumber - 1 <<": illegal symbol detected";
                                    CreateListingTrailer();
                                    exit(2);
                                    break;           
        }
    }
    if (token.length() > 15)
    {
        token = token.substr(0, 15);
    }
    
    return token;
}

char NextChar() // returns the next character or eof marker
{
    //char c;
    charac = sourceFile.get();
    if (!sourceFile)
    {
        charac = END_OF_FILE;
    }  
    
    if (charac == '\n' && newLine == false)
    {
        newLine = true;
        return charac;
    }  
    else if (charac == '\n' && newLine == true && !(tempNewLine.empty()))
    {
        listingFile << endl << setw(5) << lineNumber <<"|";
        listingFile << tempNewLine << "";
        lineNumber += 1;
        tempNewLine = "";
        return charac;
    }        
    else if (charac == '\n' && newLine == true)
    {
        listingFile << endl << setw(5) << lineNumber <<"|";
        lineNumber += 1;
        return charac;
    }
    
    if (isspace(charac) && newLine == true)
    {
        tempNewLine += charac;
        return charac;
    }
    else if (charac != ' ' && newLine == true && charac != END_OF_FILE)
    {
        listingFile << endl << setw(5) << lineNumber <<"|";
        listingFile << tempNewLine << "";
        listingFile << charac;
        lineNumber += 1;
        newLine = false;
        tempNewLine = "";
        return charac;
    }
    
    if (charac != END_OF_FILE && newLine == false)
    {
        listingFile << charac;
    }

    return charac;
}
