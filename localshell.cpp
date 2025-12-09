#include <iostream>
#include <string>
#include <sstream>
#include <unordered_set>
#include <dirent.h> //for directory operations like opendir(), readdir(), closedir()
#include <unistd.h>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <fcntl.h> //for POSIX open()
#include <readline/readline.h>
#include <readline/history.h>
#include <fstream>

using namespace std;

#ifdef _WIN32
constexpr char PATH_LIST_SEPARATOR = ';';
#else
constexpr char PATH_LIST_SEPARATOR = ':';
#endif

class BuiltinManager {
public:
    bool is_builtin(const std::string& cmd) const {
        return builtins.count(cmd);
    }

private:
    const std::unordered_set<std::string> builtins = {
        "cd", "exit", "echo", "type", "pwd", "history"
    };
};
struct RedirectInfo {
    bool redirectOut = false;
    bool redirectErr = false;
    bool appendOut = false;
    bool appendErr = false;
    bool parseError = false;

    string outFile;
    string errFile;
};

vector<string> history;
string histfile;
pid_t PARENT_PID;

vector<string> tokenize(const string &input){
  vector<string> tokens;
  bool insideQuote = false, insideDoubleQuote = false, backslash = false;
  string token = "";
  string fullInput = input;
  unordered_set<char> escape = {'"', '\\', '$', '`', '\n'};
  
  while (true) {
    for (char c : fullInput){
      if (backslash && !insideQuote){ //all the other conditions assume no backslash, we deal with quote-specific escape here
        if (insideDoubleQuote){ //inside double quotes, backslash escapes all the characters in set 'escape'
          if (escape.count(c)) token.push_back(c); //push only the character and not the backslash
          else {
            token.push_back('\\');
            token.push_back(c); //push both the character and the backslash
          }
        }
        else token.push_back(c);
        backslash = false;
        continue;
      }
      else if (c == '\'' && !insideDoubleQuote && !backslash) {
        insideQuote = !insideQuote;
        continue;
      }
      else if (c == '"' && !insideQuote && !backslash) {
        insideDoubleQuote = !insideDoubleQuote;
        continue;
      }
      else if (c == '\\'){
        if (!insideQuote){
          backslash = true;
          continue;
        }
        else {
          token.push_back(c);
          continue;
        }
      }
      if (!insideQuote && !insideDoubleQuote && !backslash){
        if (c == ' ' || c == '\t') {
          if (!token.empty()){
            tokens.push_back(token);
            token.clear();
          }
          continue;
        }
        token.push_back(c);
      }
      else {
        token += c;
      }
    }
    
    if (insideQuote || insideDoubleQuote) {
      cout << "> ";
      string nextLine;
      getline(cin, nextLine);
      fullInput = "\n" + nextLine;
      token += "\n";
    } else {
      break;
    }
  }
  
  if (!token.empty()) tokens.push_back(token);
  return tokens;
}

void echo(vector<string>& tokens){
  for (int i = 1; i < tokens.size(); i++){
    if (i > 1) cout << " ";
    cout << tokens[i];
  }
  cout << endl;
}

vector<string> splitPath(const string &path){
  vector<string> dirs;
  stringstream ss(path);
  string dir;

  while (getline(ss, dir, PATH_LIST_SEPARATOR)){
    if (!dir.empty()) dirs.push_back(dir);
  }

  return dirs;
}

string findExecutable(string &cmd){
  char* pathEnv = getenv("PATH");
  string path = pathEnv ? pathEnv : "";

  vector<string> dirs = splitPath(path);

  for (auto& dir : dirs){
    string filepath = dir + "/" + cmd;
    if (access(filepath.c_str(), F_OK) == 0){
      if (access(filepath.c_str(), X_OK) == 0){
        return filepath;
      }
    }
  }
  return "";
}

void type(vector<string>& tokens){
  string cmd;
  if (tokens.size() > 1) cmd = tokens[1];
  else cout << "type: Empty";
  BuiltinManager builtin;
  if (builtin.is_builtin(cmd))
    cout << cmd << " is a shell builtin\n";
  else {
    string found;
    found = findExecutable(cmd);
    if (!found.empty()){
      cout << cmd << " is " << found << endl;
    }
    else cout << cmd << ": not found\n";
  }
}

void pwd(){
  char buffer[PATH_MAX];

  if (getcwd(buffer, sizeof(buffer)) != nullptr){
    cout << buffer << "\n";
  }
  else {
    perror("pwd");
  }
}

void cd(const string &input){
  string path = input;

  if (path == "~"){
    char* home = getenv("HOME");
    if (home != nullptr) path = home;
    else {
      cerr << "cd: HOME not set\n";
      return;
    }
  }
  if (chdir(path.c_str()) != 0) cerr << "cd: " << path << ": No such file or directory\n";
}

void printHistory(const vector<string>& history, int n){
  int index = history.size() - n + 1;
  for (int i = history.size() - n; i < history.size(); i++){
    cout << "    " << index++ << "  " << history[i] << "\n";
  }
}
int lastAppendedIndex = 0;

void readHistoryFromFile(const string& f){
  ifstream file(f);
  if (!file.is_open()){
    cerr << "history: cannot open file\n";
    return;
  }
  string line;
  while (getline(file, line)){
    history.push_back(line);
  }
  file.close();
}

void writeHistoryToFile(const string& f){
  ofstream file(f);
  if (!file.is_open()){
    cerr << "history: cannot write to file\n";
    return;
  }
  for (auto c : history){
    file << c << "\n";
  }
  file.close();
}

void appendHistoryToFile(const string& f){
  ofstream file(f, ios::app); //ios::app represents append mode instead of the default overwrite mode.
  if (!file.is_open()){
    cerr << "history: cannot append to file\n";
    return;
  }
  for (int i = lastAppendedIndex; i < history.size(); i++) {
    file << history[i] << "\n";
  }
  file.close();
  lastAppendedIndex = history.size();
}
void dealWithHistory(const vector<string>& tokens){
  if (tokens.size() < 2) printHistory(history, history.size());
  else if (tokens.size() == 2) {
    int n = stoi(tokens[1]);

    if (n > history.size()) {cerr << "Enter a valid history size\n";}
    printHistory(history, n);
  }
  else if (tokens.size() >= 3){
    if (tokens[1] == "-r") readHistoryFromFile(tokens[2]);
    else if (tokens[1] == "-w") writeHistoryToFile(tokens[2]);
    else if (tokens[1] == "-a") appendHistoryToFile(tokens[2]);
  }
}

void dealWithBuiltin(vector<string>& tokens) {
  string cmd = tokens[0];

  if (cmd == "echo") echo(tokens);
  else if (cmd == "type") type(tokens);
  else if (cmd == "pwd") pwd();
  else if (cmd == "cd") {
      if (tokens.size() > 1) cd(tokens[1]);
      else cerr << "cd: missing argument\n";
  }
  else if (cmd == "history") dealWithHistory(tokens);
  else if (cmd == "exit") {
    if (getpid() != PARENT_PID) {
        exit(0);
    }
    throw string("__EXIT__");
  }
}

char* builtinGenerator(const char* text, int state){
  static int index;
  static const char* builtins[] = {"echo", "exit", nullptr};

  if (state == 0) index = 0;
  while(builtins[index]){
    const char* name = builtins[index++];
    if (strncmp(name, text, strlen(text)) == 0) return strdup(name);
  }
  return nullptr;
}

vector<string> findExecutableByPath(const string& prefix){
  vector<string> matches;
  char* pathEnv = getenv("PATH");
  string path = pathEnv ? pathEnv : "";
  vector<string> dirs = splitPath(path);

  for (auto& dir : dirs){
    DIR* dp = opendir(dir.c_str());
    if (!dp) continue;

    struct dirent* entry; //This line declares a pointer that points to whatever directory readdir() returns.
    //structure defined in dirent.h that represents one directory entry inside a directory. Struct has various fields like name, offset, length, etc.
    while ((entry = readdir(dp)) != nullptr){
      string filename = entry->d_name;
      if (filename.rfind(prefix, 0) != 0) continue; //if the filename doesn't start with prefix.
      if (filename == "." || filename == "..") continue;

      string fullpath = dir + "/" + filename;
      if (access(fullpath.c_str(), X_OK) == 0) matches.push_back(filename);
    }
    closedir(dp);
  }
  return matches;
}

char* generatorFunction(const char* text, int state){
  static vector<string> allMatches;
  static size_t idx;

  if (state == 0){
    allMatches.clear();
    idx = 0;
    const char* builtins[] = {"echo", "exit", nullptr};
    for (int i = 0; builtins[i] != nullptr; i++){
      if (strncmp(builtins[i], text, strlen(text)) == 0){
        allMatches.push_back(builtins[i]);
      }
    }
    auto ext = findExecutableByPath(text);
    allMatches.insert(allMatches.end(), ext.begin(), ext.end()); //all possible matches, builtin or otherwise.
  }
  if (idx >= allMatches.size()) return NULL;
  return strdup(allMatches[idx++].c_str());
  //basically, readline calls this function multiple times to MANUALLY build its char** array with the options we return one by one from here.
  //state represents the number of calls already made (ie, the number of elements already read).
  //variables need to be static because they must be constant across all calls.
}

char** completion(const char* text, int start, int end){
  if (start == 0) return rl_completion_matches(text, generatorFunction);
  rl_attempted_completion_over = 1;
  return nullptr;
}

void checkRedirect(vector<string>& tokens, RedirectInfo& rinfo, bool& parseError){
  for (int i = 0; i < tokens.size(); i++){
    if (tokens[i] == ">" || tokens[i] == "1>"){
      rinfo.redirectOut = true;
      if (i + 1 >= tokens.size()) { cerr << "missing file\n"; parseError = true; break; }
      rinfo.outFile = tokens[i+1];
      tokens.erase(tokens.begin() + i, tokens.begin() + i + 2);
      continue;
    }
    else if (tokens[i] == "2>"){
      rinfo.redirectErr = true;
      if (i + 1 >= tokens.size()) { cerr << "missing file\n"; parseError = true; break; }
      rinfo.errFile = tokens[i+1];
      tokens.erase(tokens.begin() + i, tokens.begin() + i + 2);
      continue;
    }
    else if (tokens[i] == ">>" || tokens[i] == "1>>"){
      rinfo.redirectOut = true;
      rinfo.appendOut = true;
      if (i + 1 >= tokens.size()) { cerr << "missing file\n"; parseError = true; break; }
      rinfo.outFile = tokens[i+1];
      tokens.erase(tokens.begin() + i, tokens.begin() + i + 2);
      continue;
    }
    else if (tokens[i] == "2>>"){
      rinfo.redirectErr = true;
      rinfo.appendErr = true;
      if (i + 1 >= tokens.size()) { cerr << "missing file\n"; parseError = true; break; }
      rinfo.errFile = tokens[i+1];
      tokens.erase(tokens.begin() + i, tokens.begin() + i + 2);
      continue;
    }
  }
}

void dealWithPipeline(vector<vector<string>>& pipecmds, vector<RedirectInfo>& rinfos, int pipes){
  int fds[pipes][2]; //fds[1] -> write end. fds[0] -> read end.
  vector<pid_t> pids(pipes + 1);
  for (int i = 0; i < pipes; i++){
    if (pipe(fds[i]) < 0){
      perror("pipe failed");
      exit(1);
    }
  }
  for (int i = 0; i < pipes + 1; i++){
    pids[i] = fork();

    if (pids[i] == 0){

      if (i > 0) dup2(fds[i-1][0], 0);
      if (i < pipes) dup2(fds[i][1], 1);

      for (int p = 0; p < pipes; p++) {
        if (p == i - 1) close(fds[p][1]);
        else if (p == i) close(fds[p][0]);
        else {
            close(fds[p][0]);
            close(fds[p][1]);
        }
      }
      if (rinfos[i].redirectErr) {
        int flags = O_WRONLY | O_CREAT | (rinfos[i].appendErr ? O_APPEND : O_TRUNC);
        int fd = open(rinfos[i].errFile.c_str(), flags, 0644);
        dup2(fd, 2);
        close(fd);
      }
      if (i < pipes) {
        if (rinfos[i].redirectOut){
          cerr << "Redirection not allowed here" << "\n";
          exit(1);
        }
      }
      else {
        if (rinfos[i].redirectOut) {
          int flags = O_WRONLY | O_CREAT | (rinfos[i].appendOut ? O_APPEND : O_TRUNC);
          int fd = open(rinfos[i].outFile.c_str(), flags, 0644);
          dup2(fd, 1);
          close(fd);
        }
      }

      vector<char*> argv;
      string cmd = pipecmds[i][0];
      BuiltinManager builtin;
      if (builtin.is_builtin(cmd)){
        dealWithBuiltin(pipecmds[i]);
        exit(0);
      }
      for (auto &t : pipecmds[i]){
        argv.push_back(const_cast<char*>(t.c_str()));
      }
      argv.push_back(NULL);

      execvp(cmd.c_str(), argv.data());
      cout << cmd << ": command not found" << endl;
      exit(1);
    }
    else if (pids[i] < 0){
      perror("fork failed");
      exit(1);
    }
  }
  for (int i = 0; i < pipes; i++){
    close(fds[i][0]);
    close(fds[i][1]);
  }
  for (int i = 0; i < pipes + 1; i++) waitpid(pids[i], nullptr, 0);
}

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  rl_attempted_completion_function = completion;
  rl_completion_append_character = ' ';
  char* hf = getenv("HISTFILE");
  histfile = (hf ? string(hf) : "");

  if (hf) readHistoryFromFile(histfile);
  PARENT_PID = getpid();

  while (true){
    char* input = readline("$ ");

    if (!input) continue;
    string command = string(input);
    if (*input) add_history(input);
    free(input);

    RedirectInfo rinfo;
    bool pipeline = false;
    vector<vector<string>> pipecmds;
    
    history.push_back(command);
    
    vector<string> tokens = tokenize(command);
    if (tokens.empty()) continue;
    string cmd = tokens[0];
    int pipes = 0, start = 0;

    for (int i = 0; i < tokens.size(); i++){
      vector<string> tempcmd;
      if (tokens[i] == "|"){
        if (i == 0 || i == tokens.size() - 1) {
          cerr << "syntax error near unexpected token '|'\n";
          rinfo.parseError = true;
          break;
        }
        pipes++;
        pipeline = true;
        tempcmd.assign(tokens.begin() + start, tokens.begin() + i);
        pipecmds.push_back(tempcmd);
        start = i + 1;
      }
    }
    if (pipeline){
      vector<string> tempcmd;
      tempcmd.assign(tokens.begin() + start, tokens.end());
      pipecmds.push_back(tempcmd);  // Push the last command

      vector<RedirectInfo> rinfos(pipes + 1);
      int sum = 0;
      for (int i = 0; i < pipecmds.size(); i++){
        checkRedirect(pipecmds[i], rinfos[i], rinfos[i].parseError);
        sum += rinfos[i].parseError;
      }
      if (sum > 0) continue;
      dealWithPipeline(pipecmds, rinfos, pipes);
      continue;
    }
    
    checkRedirect(tokens, rinfo, rinfo.parseError);

    if (rinfo.parseError) continue;
    BuiltinManager builtin;
    if (builtin.is_builtin(cmd)){
      if (rinfo.redirectOut || rinfo.redirectErr){
        pid_t pid = fork();
        if (pid < 0){
          perror("fork failed\n");
          exit(1);
        }
        if (pid == 0) {
          if (rinfo.redirectOut) {
            int flags = O_WRONLY | O_CREAT | (rinfo.appendOut ? O_APPEND : O_TRUNC);
            //O_WRONLY -> open for writing. O_CREAT -> Create if doesn't exist. O_TRUNC -> Overwrite if exists. 0644 -> read/write for user.
            int fd = open(rinfo.outFile.c_str(), flags, 0644);
            dup2(fd, 1); //0 -> stdin; 1 -> stdout; 2 -> stderror. So this redirects stdout -> file.
            close(fd); //fd not needed anymore.
          }
          if (rinfo.redirectErr) {
            int flags = O_WRONLY | O_CREAT | (rinfo.appendErr ? O_APPEND : O_TRUNC);
            int fd = open(rinfo.errFile.c_str(), flags, 0644);
            dup2(fd, 2); //0 -> stdin; 1 -> stdout; 2 -> stderror. So this redirects stderr -> file.
            close(fd);
          }
          dealWithBuiltin(tokens);
          exit(0);
        }

        waitpid(pid, nullptr, 0);
        continue;
      }
      try {
        dealWithBuiltin(tokens);
      } catch (string &msg) {
        if (msg == "__EXIT__") break;
      }
      continue;
    }
    else {
      pid_t pid = fork(); //create a fork (which returns the Process ID of the child process).
      if (pid < 0){
        perror("fork failed\n");
        exit(1);
      }
      else if (pid == 0){
        if (rinfo.redirectOut) {
          int flags = O_WRONLY | O_CREAT | (rinfo.appendOut ? O_APPEND : O_TRUNC);

          int fd = open(rinfo.outFile.c_str(), flags, 0644);
          if (fd == -1) {
            cerr << cmd << ": nonexistent: No such file or directory\n";
            exit(1);
          }
          dup2(fd, 1);   // redirect stdout
          close(fd);
        }
        if (rinfo.redirectErr) {
          int flags = O_WRONLY | O_CREAT | (rinfo.appendErr ? O_APPEND : O_TRUNC);

          int fd = open(rinfo.errFile.c_str(), flags, 0644);

          if (fd == -1) {
            cerr << cmd << ": nonexistent: No such file or directory\n";
            exit(1);
          }
          dup2(fd, 2);   // redirect stderr
          close(fd);
        }
        vector<char*> argv;
        for (auto &t : tokens){
          argv.push_back(const_cast<char*>(t.c_str())); //as argv must be of the form const char*[].
        }
        argv.push_back(NULL); //command line argument array must be null-terminated.

        execvp(cmd.c_str(), argv.data()); //argv.data() returns a char** pointer -> what execvp wants, and
        //cmd.c_str() because the parameter expects a C-style char* string, not a C++ string.
        cout << cmd << ": command not found" << endl;
        exit(1);
      }
      else {
        int status;
        waitpid(pid, &status, 0); //(pid from fork(), pointer for child's exit cause, option)
      }
    }
  }
  if (!histfile.empty()) writeHistoryToFile(histfile);
  return 0;
}