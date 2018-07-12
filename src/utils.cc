#include "utils.hh"
#include <stdio.h>
#include <assert.h>
#include <sys/utsname.h>


Logger::Logger(double c, size_t nrules, int verbosity, char* log_fname, int freq) {
      _c = c;
      _nrules = nrules - 1;
      _v = verbosity;
      _freq = freq;
      setLogFileName(log_fname);
      initPrefixVec();
}

/*
 * Sets the logger file name and writes the header line to the file.
 */
void Logger::setLogFileName(char *fname) {
    if (_v < 1) return;

    printf("writing logs to: %s\n\n", fname);
    _f.open(fname, ios::out | ios::trunc);

    _f << "total_time,evaluate_children_time,node_select_time,"
       << "rule_evaluation_time,lower_bound_time,lower_bound_num,"
       << "objective_time,objective_num,"
       << "tree_insertion_time,tree_insertion_num,queue_insertion_time,evaluate_children_num,"
       << "permutation_map_insertion_time,permutation_map_insertion_num,permutation_map_memory,"
       << "current_lower_bound,tree_min_objective,tree_prefix_length,"
       << "tree_num_nodes,tree_num_evaluated,tree_memory,"
       << "queue_size,queue_min_length,queue_memory,"
       << "pmap_size,pmap_null_num,pmap_discard_num,"
       << "log_remaining_space_size,prefix_lengths" << endl;
}

/*
 * Writes current stats about the execution to the log file.
 */
void Logger::dumpState() {
    if (_v < 1) return;

    // update timestamp here
    setTotalTime(time_diff(_state.initial_time));

    _f << _state.total_time << ","
       << _state.evaluate_children_time << ","
       << _state.node_select_time << ","
       << _state.rule_evaluation_time << ","
       << _state.lower_bound_time << ","
       << _state.lower_bound_num << ","
       << _state.objective_time << ","
       << _state.objective_num << ","
       << _state.tree_insertion_time << ","
       << _state.tree_insertion_num << ","
       << _state.queue_insertion_time << ","
       << _state.evaluate_children_num << ","
       << _state.permutation_map_insertion_time << ","
       << _state.permutation_map_insertion_num << ","
       << _state.pmap_memory << ","
       << _state.current_lower_bound << ","
       << _state.tree_min_objective << ","
       << _state.tree_prefix_length << ","
       << _state.tree_num_nodes << ","
       << _state.tree_num_evaluated << ","
       << _state.tree_memory << ","
       << _state.queue_size << ","
       << _state.queue_min_length << ","
       << _state.queue_memory << ","
       << _state.pmap_size << ","
       << _state.pmap_null_num << ","
       << _state.pmap_discard_num << ","
       << getLogRemainingSpaceSize() << ","
       << dumpPrefixLens().c_str() << endl;
}

/*
 * Uses GMP library to dump a string version of the remaining state space size.
 * This number is typically very large (e.g. 10^20) which is why we use GMP instead of a long.
 * Note: this function may not work on some Linux machines.
 */
std::string Logger::dumpRemainingSpaceSize() {
    mpz_class s(_state.remaining_space_size);
    return s.get_str();
}

/*
 * Function to convert vector of remaining prefix lengths to a string format for logging.
 */
std::string Logger::dumpPrefixLens() {
    std::string s = "";
    for(size_t i = 0; i < _nrules; ++i) {
        if (_state.prefix_lens[i] > 0) {
            s += std::to_string(i);
            s += ":";
            s += std::to_string(_state.prefix_lens[i]);
            s += ";";
        }
    }
    return s;
}

/*
 * Given a rulelist and predictions, will output a human-interpretable form to a file.
 */

void print_final_rulelist(const tracking_vector<unsigned short, DataStruct::Tree>& rulelist,
                          const tracking_vector<bool, DataStruct::Tree>& preds,
                          const bool latex_out,
                          const rule_t rules[],
                          const rule_t labels[],
                          char fname[],
                          int print_progress, bool print_debug,
                          int nsamples, int nrules);

void print_final_rulelist(const tracking_vector<unsigned short, DataStruct::Tree>& rulelist,
                          const tracking_vector<bool, DataStruct::Tree>& preds,
                          const bool latex_out,
                          const rule_t rules[],
                          const rule_t labels[],
                          char fname[],
                          int print_progress)
{
    print_final_rulelist(rulelist, preds, latex_out, rules, labels, fname, print_progress, false, 0, 0);
}

void print_final_rulelist(const tracking_vector<unsigned short, DataStruct::Tree>& rulelist,
                          const tracking_vector<bool, DataStruct::Tree>& preds,
                          const bool latex_out,
                          const rule_t rules[],
                          const rule_t labels[],
                          char fname[],
                          int print_progress, int nsamples, int nrules)
{
    print_final_rulelist(rulelist, preds, latex_out, rules, labels, fname, print_progress, true, nsamples, nrules);
}

void print_final_rulelist(const tracking_vector<unsigned short, DataStruct::Tree>& rulelist,
                          const tracking_vector<bool, DataStruct::Tree>& preds,
                          const bool latex_out,
                          const rule_t rules[],
                          const rule_t labels[],
                          char fname[],
                          int print_progress, bool print_debug,
                          int nsamples, int nrules) {
    assert(rulelist.size() == preds.size() - 1);

    // Generate ruleset
    /*
    ruleset_t *ruleset;
    rule_t rules_copy[nrules];
    int idarray[rulelist.size()];
    for (size_t i = 0; i < rulelist.size(); i++)
        idarray[i] = rulelist[i];
    for (int i = 0; i < nrules; i++)
        rules_copy[i] = rules[i];
    ruleset_init(rulelist.size(), nsamples, idarray, rules_copy, &ruleset);
    */
    
    // Print entire rules array
    printf("\nALL RULE LIST\n");
    for (size_t i = 0; i < rulelist.size(); i++) {
        printf("%d ", rulelist[i]);
    }
    printf("\n");
    for (int i = 0; i < nrules; i++) {
        printf("if (%s) ", rules[i].features);
        printf("%d %d\n", rules[i].support, rules[i].cardinality);
        //printf("    %d %d\n", rules[rulelist[i]].support, rules[rulelist[i]].cardinality);
        //sum += rules[rulelist[i]].support;
    }
    
    printf("\nOPTIMAL RULE LIST\n");
    if (rulelist.size() > 0) {
        int sum = 0;
        printf("if (%s) then (%s)\n", rules[rulelist[0]].features,
               labels[preds[0]].features);
        printf("    %d %d\n", rules[rulelist[0]].support, rules[rulelist[0]].cardinality);
        sum += rules[rulelist[0]].support;
        for (size_t i = 1; i < rulelist.size(); ++i) {
            printf("else if (%s) then (%s)\n", rules[rulelist[i]].features,
                   labels[preds[i]].features);
            printf("    %d %d\n", rules[rulelist[i]].support, rules[rulelist[i]].cardinality);
            sum += rules[rulelist[i]].support;
        }
        printf("else (%s)\n\n", labels[preds.back()].features);
        printf("nsamples: %d\n\n", nsamples);
        printf("sum: %d\n\n", sum);
        printf("Support:\n");
        for (int i = 0; i < nrules; i++)
            printf("%d ", rules[i].support);
        printf("\n\nCardinality:\n");
        for (int i = 0; i < nrules; i++)
            printf("%d ", rules[i].cardinality);
        printf("\n\n");
        
        if (latex_out) {
            printf("\nLATEX form of OPTIMAL RULE LIST\n");
            printf("\\begin{algorithmic}\n");
            printf("\\normalsize\n");
            printf("\\State\\bif (%s) \\bthen (%s)\n", rules[rulelist[0]].features,
                   labels[preds[0]].features);
            for (size_t i = 1; i < rulelist.size(); ++i) {
                printf("\\State\\belif (%s) \\bthen (%s)\n", rules[rulelist[i]].features,
                       labels[preds[i]].features);
            }
            printf("\\State\\belse (%s)\n", labels[preds.back()].features);
            printf("\\end{algorithmic}\n\n");
        }
    } else {
        printf("if (1) then (%s)\n\n", labels[preds.back()].features);

        if (latex_out) {
            printf("\nLATEX form of OPTIMAL RULE LIST\n");
            printf("\\begin{algorithmic}\n");
            printf("\\normalsize\n");
            printf("\\State\\bif (1) \\bthen (%s)\n", labels[preds.back()].features);
            printf("\\end{algorithmic}\n\n");
        }
    }

    ofstream f;
    printf("writing optimal rule list to: %s\n\n", fname);
    f.open(fname, ios::out | ios::trunc);
    for(size_t i = 0; i < rulelist.size(); ++i) {
        f << rules[rulelist[i]].features << "~"
          << preds[i] << ";";
    }
    f << "default~" << preds.back();
    f.close();
}

/*
 * Prints out information about the machine.
 */
void print_machine_info() {
    struct utsname buffer;

    if (uname(&buffer) == 0) {
        printf("System information:\n"
               "system name-> %s; node name-> %s; release-> %s; "
               "version-> %s; machine-> %s\n\n",
               buffer.sysname,
               buffer.nodename,
               buffer.release,
               buffer.version,
               buffer.machine);
    }
}
