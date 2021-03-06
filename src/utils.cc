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

void calculate_scores(double *scores, int nsamples, VECTOR captured, bool wpa, size_t ruleindex, double proportion)
{
    for (int i = 0; i < nsamples; i++) {
        if (rule_isset(captured, i)) {
            if (wpa)
                *(scores+i) = -1 * (int)ruleindex;
            else
                *(scores+i) = proportion;
        }
    }
}

/*
 * Given a rulelist and predictions, will output a human-interpretable form to a file.
 */
// Function declaration of core function
void process_and_print_final_rulelist(
    const tracking_vector<unsigned short, DataStruct::Tree>& rulelist, 
    const tracking_vector<bool, DataStruct::Tree>& preds, 
    const bool latex_out, 
    const rule_t rules[], 
    const rule_t labels[], 
    char fname[], 
    int print_progress, 
    bool print_debug = false, 
    bool wpa = false, 
    int nsamples = 0, 
    int nrules = 0, 
    double *retscores = NULL, 
    double c = 0, 
    double *retaccuracy = NULL);

// Wrapper functions for core function
void print_final_rulelist(const tracking_vector<unsigned short, DataStruct::Tree>& rulelist,
                          const tracking_vector<bool, DataStruct::Tree>& preds,
                          const bool latex_out,
                          const rule_t rules[],
                          const rule_t labels[],
                          char fname[],
                          int print_progress)
{
    process_and_print_final_rulelist(rulelist, preds, latex_out, rules, labels, fname, print_progress);
}

void process_and_print_final_rulelist(
    const tracking_vector<unsigned short, DataStruct::Tree>& rulelist,
    const tracking_vector<bool, DataStruct::Tree>& preds,
    const bool latex_out,
    const rule_t rules[],
    const rule_t labels[],
    char fname[],
    int print_progress,
    bool wpa,
    int nsamples,
    int nrules,
    double *retscores,
    double c,
    double *retaccuracy)
{
    process_and_print_final_rulelist(rulelist, preds, latex_out, rules, labels, fname, print_progress, true, wpa, nsamples, nrules, retscores, c, retaccuracy);
}
// Core function implementation
void process_and_print_final_rulelist(
    const tracking_vector<unsigned short, DataStruct::Tree>& rulelist,
    const tracking_vector<bool, DataStruct::Tree>& preds,
    const bool latex_out,
    const rule_t rules[],
    const rule_t labels[],         
    char fname[],
    int print_progress,
    bool print_debug,
    bool wpa,
    int nsamples,
    int nrules,
    double *retscores,
    double c,
    double *retaccuracy)
{
    assert(rulelist.size() == preds.size() - 1);
    
    // Print entire rules array
    // if (print_debug) {
    //     printf("\nALL RULE LIST\n");
    //     /*
    //     for (size_t i = 0; i < rulelist.size(); i++) {
    //         //printf("%d %d", rulelist[i], preds[i]);
    //         rule_t label = labels[preds[i]];
    //         printf("%s %d %d\n", label.features, label.support, label.cardinality);
    //     }
    //     */
    //     for (int i = 0; i < 2; i++)
    //         printf("%s %d %d\n", labels[i].features, labels[i].support, labels[i].cardinality);
    //     //printf("\n");
    //     for (int i = 0; i < nrules; i++) {
    //         printf("if (%s) ", rules[i].features);
    //         printf("%d %d\n", rules[i].support, rules[i].cardinality);
    //     }
    // }
    double accuracy = 0;
    printf("\nOPTIMAL RULE LIST\n");
    //double objective = labels[0].support * labels[1].support;
    //printf("initial obj: %f\n", objective);
    if (rulelist.size() > 0) {
        if (print_debug) {
            printf("***********\n");
            printf("FORMAT:\nif ({rule}) then ({label})\n");
            printf("    support cardinality ncaptured proportion\n");
            printf("***********\n");
        }
        printf("if (%s) then (%s)\n", rules[rulelist[0]].features,
               labels[preds[0]].features);
  
        VECTOR captured, total_captured;
        rule_vinit(nsamples, &captured);
        rule_vinit(nsamples, &total_captured);
        rule_t curr_rule = rules[rulelist[0]]; // Make a copy to remove const qualifier
        rule_copy(captured, curr_rule.truthtable, nsamples);
        rule_copy(total_captured, captured, nsamples);
        int ncaptured = rules[rulelist[0]].support; // Only for first rule
        int total_ncaptured = ncaptured;
        int support_sum = 0, captured_sum = 0;
        // Make copy to remove const
        rule_t ones_label = labels[1];
        rule_t zeros_label = labels[0];
        // Count number of ones
        int num_ones, d0;
        VECTOR ones, default_zeros;
        rule_vinit(nsamples, &ones);
        rule_vinit(nsamples, &default_zeros);
        rule_vand(ones, captured, ones_label.truthtable, nsamples, &num_ones);
        rule_vandnot(default_zeros, zeros_label.truthtable, total_captured, nsamples, &d0);
        double proportion = (double)num_ones/ncaptured;
        double max_proportion = (proportion < 1 - proportion) ? 1 - proportion : proportion;
        //objective -= num_ones * d0 - c;
        //printf("%d %d %d %f %f\n", num_ones, d0, d0, c, objective);
        accuracy += max_proportion * ncaptured / nsamples;
        if (retscores != NULL)
            calculate_scores(retscores, nsamples, captured, wpa, 0, proportion);
        if (print_debug) {
            printf("    %d %d %d %f\n", rules[rulelist[0]].support, rules[rulelist[0]].cardinality, ncaptured, proportion);
        }
        support_sum += rules[rulelist[0]].support;
        captured_sum += ncaptured;
        for (size_t i = 1; i < rulelist.size(); ++i) {
            curr_rule = rules[rulelist[i]];
            rule_vandnot(captured, curr_rule.truthtable, total_captured, nsamples, &ncaptured);
            rule_vor(total_captured, curr_rule.truthtable, total_captured, nsamples, &total_ncaptured);
            printf("else if (%s) then (%s)\n", rules[rulelist[i]].features,
                   labels[preds[i]].features);
            int support = rules[rulelist[i]].support;
            // Count number of 1's
            rule_vand(ones, captured, ones_label.truthtable, nsamples, &num_ones);
            double proportion = (double)num_ones/ncaptured;
            double max_proportion = (proportion < 1 - proportion) ? 1 - proportion : proportion;
            rule_vandnot(default_zeros, zeros_label.truthtable, total_captured, nsamples, &d0);
            //objective -= num_ones * d0 - c;
            accuracy += max_proportion * ncaptured / nsamples;
            if (retscores != NULL)
                calculate_scores(retscores, nsamples, captured, wpa, i, proportion);
            if (print_debug) {
                printf("    %d %d %d %f\n", support, rules[rulelist[i]].cardinality, ncaptured, proportion);
                /*
                for (int i = 0; i < nsamples; i++)
                    printf("%d", rule_isset(captured, i));
                printf("\n");
                */
            }
            assert(ncaptured <= support);
            support_sum += rules[rulelist[i]].support;
            captured_sum += ncaptured;
        }
        printf("else (%s)\n", labels[preds.back()].features);
        rule_not(captured, total_captured, nsamples, &ncaptured);
        // Recalculate ncaptured because rule_not() does not handle 2's complement
        ncaptured = nsamples - total_ncaptured;
        rule_vandnot(ones, ones_label.truthtable, total_captured, nsamples, &num_ones);
        proportion = (double)num_ones/ncaptured;
        max_proportion = (proportion < 1 - proportion) ? 1 - proportion : proportion;
        accuracy += max_proportion * ncaptured / nsamples;
        if (retscores != NULL)
            calculate_scores(retscores, nsamples, captured, wpa, rulelist.size(), proportion);
        if (print_debug)
            printf("    %d %d %d %f\n\n", ncaptured, 0, ncaptured, proportion);
        else
            printf("\n");
        
        if (print_debug) {
            printf("nsamples: %d\n\n", nsamples);
            printf("support_sum: %d\n\n", support_sum);
            printf("captured_sum: %d\n\n", captured_sum);
            /*
            printf("Support:\n");
            for (int i = 0; i < nrules; i++)
                printf("%d ", rules[i].support);
            printf("\n\nCardinality:\n");
            for (int i = 0; i < nrules; i++)
                printf("%d ", rules[i].cardinality);
            printf("\n\n");
            */
        }
        
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
        accuracy = (double)labels[preds[0]].support/nsamples;

        if (latex_out) {
            printf("\nLATEX form of OPTIMAL RULE LIST\n");
            printf("\\begin{algorithmic}\n");
            printf("\\normalsize\n");
            printf("\\State\\bif (1) \\bthen (%s)\n", labels[preds.back()].features);
            printf("\\end{algorithmic}\n\n");
        }
    }

    if (retaccuracy != NULL)
        *retaccuracy = accuracy;

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
