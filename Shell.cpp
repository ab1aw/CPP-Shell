///////////////////////////
// Tanner Kvarfordt      //
// A02052217             //
// CS 3100 --Dr. Mathias //
// Assignment 3          //
///////////////////////////

#include "Shell.hpp"

Shell::Shell() :
    M_CMD_DELIMITER(" "), M_EXIT_CMD("exit"),
    M_HISTORY("history"), M_RUN_HISTORY("^"),
    M_PTIME("ptime"),
    M_INCLUDE_RUN_HISTORY(false),
    m_history(), m_child_time_total(0) {}

Shell::Shell(bool const & include_run_hist) :
    M_CMD_DELIMITER(" "), M_EXIT_CMD("exit"),
    M_HISTORY("history"), M_RUN_HISTORY("^"),
    M_PTIME("ptime"),
    M_INCLUDE_RUN_HISTORY(include_run_hist),
    m_history(), m_child_time_total(0) {}

Shell::Shell(bool const & include_run_hist, std::string const & run_hist_cmd) :
    M_CMD_DELIMITER(" "), M_EXIT_CMD("exit"),
    M_HISTORY("history"), M_RUN_HISTORY(run_hist_cmd),
    M_PTIME("ptime"),
    M_INCLUDE_RUN_HISTORY(include_run_hist),
    m_history(), m_child_time_total(0) {}

Shell::Shell(bool const & include_run_hist, std::string const & run_hist_cmd,
             std::string const & hist_cmd, std::string const & exit_cmd,
             std::string const & ptime, std::string const cmd_delim) :
    M_CMD_DELIMITER(cmd_delim), M_EXIT_CMD(exit_cmd),
    M_HISTORY(hist_cmd), M_RUN_HISTORY(run_hist_cmd),
    M_INCLUDE_RUN_HISTORY(include_run_hist),
    M_PTIME(ptime) {}

void Shell::run() {
    std::vector<std::string> input_args;
    std::string input = "";
    while (true) {
        input_args.clear();
        input = prompt();
        if (input != "") {
            m_history.push_back(input);
            parse_string(m_history.back(), M_CMD_DELIMITER, input_args);
        }

        if (input_args.size() > 0) {

            if (!M_INCLUDE_RUN_HISTORY) {
                // If command is M_RUN_HISTORY, remove it from m_history to prevent confusion
                if (input_args.at(0) == M_RUN_HISTORY) m_history.pop_back();
            }

            // Check if command is built-in or the M_EXIT_CMD
            if (input_args.at(0) == M_EXIT_CMD) return;
            if (!isBuiltIn(input_args)) {
                // Otherwise, send it to OS
                if (fork()) {
                    // parent process executes here

                    // wait for child process to terminate before continuing
                    m_child_time_total += timeChild();
                } else {
                    // child process executes here

                    exec_cmd(input_args);
                    // remove all chance of fork bomb
                    return;
                    // can't be too careful...
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
}

// Prompts a user for input and returns the result
std::string Shell::prompt() const {
    std::cout << "[cmd]: ";
    std::string input = "";
    std::getline(std::cin, input);
    return input;
}

// @input_args contains the user's command and all of its arguments.
// This function translates the command its arguments into cstrings,
// then passes them off to the OS via execvp to be executed
void Shell::exec_cmd(std::vector<std::string> const &input_args) const {
    char** argv = new char*[input_args.size() + 1];
    for (unsigned int i = 0; i < input_args.size(); ++i) {
        // cstrings require a null terminator, hence the + 1
        argv[i] = new char[input_args.at(i).length() + 1];
        strncpy(argv[i], input_args[i].c_str(), input_args[i].size() + 1);
    }
    // NULL to let execvp know there are no more args
    argv[input_args.size()] = NULL;
    execvp(argv[0], argv);
    std::cout << input_args.at(0) << ": command not found" << std::endl;
    // protect against fork bomb
    exit(EXIT_FAILURE);
}

// Prints the contents of m_history
void Shell::printHistory() const {
    std::cout << std::endl << "-- Command History --" << std::endl << std::endl;
    for (unsigned int i = 0; i < m_history.size(); ++i) {
        std::cout << std::setw(std::to_string(m_history.size()).length()) << i << " : " << m_history.at(i) << std::endl;
    }
    std::cout << std::endl;
}

// Runs the command in m_history[@entry]
// Returns the amount of time spent running a child process, if any
Duration Shell::runHistoryEntry(std::string const &entry) {
    // check for non-int type
    if (entry.find('.') != std::string::npos) return Duration(0);

    int entry_int;
    try {
        entry_int = std::stoi(entry);
        // check that history index is in range
        if (entry_int < 0 || entry_int > m_history.size() - 1) return Duration(0);
    } catch (std::exception e) {
        // invalid history index
        return Duration(0);
    }

    // parse the input_args from the history index
    std::vector<std::string> input_args;
    parse_string(m_history.at(entry_int), M_CMD_DELIMITER, input_args);

    // Check for built in
    if (isBuiltIn(input_args)) {
        return Duration(0);
    } else {
        // Otherwise, send it to the OS
        if (fork()) {
            // parent process executes here

            return timeChild();
        } else {
            // child process executes here

            exec_cmd(input_args);

            // protect against fork bomb
            exit(EXIT_FAILURE);
        }
    }
}

// Checks to see if the command in @input_args is built-in
// If so, it calls the appropriate function
// If not, it returns false
bool Shell::isBuiltIn(std::vector<std::string> const &input_args) {

    if (input_args.at(0) == M_HISTORY) {
        printHistory();
        return true;
    }
    if (input_args.at(0) == M_RUN_HISTORY && input_args.size() > 1) {
        m_child_time_total += runHistoryEntry(input_args.at(1));
        return true;
    }
    if (input_args.at(0) == M_PTIME) {
        printPtime();
        return true;
    }
    return false;
}

void Shell::printPtime() const {
    std::cout << std::endl << "Time spent executing child processes: "
              << std::fixed << std::setprecision(4)
              << m_child_time_total.count() << " seconds" << std::endl << std::endl;
}

// Returns the amount of time spent waiting on a child process to finish executing
Duration Shell::timeChild() const {
    Time_Point start = High_Res_Clock::now();
    wait(NULL);
    Time_Point end = High_Res_Clock::now();
    return end - start;
}


