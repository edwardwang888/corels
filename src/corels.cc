#include "queue.hh"
#include <algorithm>
#include <iostream>
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>

Queue::Queue(std::function<bool(Node*, Node*)> cmp, char const *type)
    : q_(new q (cmp)), type_(type) {}

bool has_falling_constraint(double proportion, double parent_proportion, double default_proportion) {
    return (parent_proportion == 0 || parent_proportion > proportion) && proportion > default_proportion;
}

int random_search(double p)
{
    return random() <= p * RAND_MAX;
}

int compare_doubles(const void *a, const void *b)
{
    const double *da =  (const double *)a;
    const double *db = (const double *)b;
    return (int)(*da - *db);
}

int count_greater(VECTOR captured, int ncaptured, VECTOR ones_label, int nsamples)
{
    int y[ncaptured];
    int len = 0;
    // printf("%d %d\n", ncaptured, count_ones_vector(captured, nsamples));
    for (int i = 0; i < nsamples; i++) {
        // printf("%d ", len);
        if (rule_isset(captured, i)) {
            y[len] = rule_isset(ones_label, i);
            len++;
        }
    }
    if (len != ncaptured)
        printf("\nnot equal %d %d\n", len, ncaptured);
    int count = 0;
    for (int i = 0; i < ncaptured; i++)
        for (int j = 0; j < ncaptured; j++)
            count += (y[i] > y[j]);
    return count;
}

/*
 * Performs incremental computation on a node, evaluating the bounds and inserting into the cache,
 * queue, and permutation map if appropriate.
 * This is the function that contains the majority of the logic of the algorithm.
 *
 * parent -- the node that is going to have all of its children evaluated.
 * parent_not_captured -- the vector representing data points NOT captured by the parent.
 */
void evaluate_children(CacheTree* tree, Node* parent, tracking_vector<unsigned short, DataStruct::Tree> parent_prefix, VECTOR parent_not_captured, Queue* q, PermutationMap* p, bool falling, bool show_proportion, bool change_search_path, double ties, double random, double bound) {
    VECTOR captured, captured_zeros, not_captured, not_captured_zeros, not_captured_equivalent;
    int num_captured, c0, c1, captured_correct;
    int num_not_captured, d0, d1, default_correct, num_not_captured_equivalent;
    bool prediction, default_prediction;
    double lower_bound, objective, parent_lower_bound, lookahead_bound;
    double parent_equivalent_minority;
    double equivalent_minority = 0.;
    int nsamples = tree->nsamples();
    int nrules = tree->nrules();
    double c = tree->c();
    double threshold = c * nsamples;
    rule_vinit(nsamples, &captured);
    rule_vinit(nsamples, &captured_zeros);
    rule_vinit(nsamples, &not_captured);
    rule_vinit(nsamples, &not_captured_zeros);
    rule_vinit(nsamples, &not_captured_equivalent);
    int i, len_prefix;
    len_prefix = parent->depth() + 1;
    parent_lower_bound = parent->lower_bound();
    parent_equivalent_minority = parent->equivalent_minority();
    double t0 = timestamp();

    int total_zeros = tree->label(0).support;
    int total_ones = nsamples - total_zeros;


    // nrules is actually the number of rules + 1 (since it includes the default rule), so the maximum
    // value of i is nrules - 1 instead of nrules
    std::set<std::string> verbosity = logger->getVerbosity();
    // Have two versions depending on whether wpa is true or not (a lot of copying and pasting)
    /************************************** 
    *** Version to run when wpa is FALSE ***
    **************************************/
    if (tree->wpa() == false) {
        for (i = 1; i < nrules; i++) {
            double t1 = timestamp();
            // check if this rule is already in the prefix
            if (std::find(parent_prefix.begin(), parent_prefix.end(), i) != parent_prefix.end())
                continue;
            // captured represents data captured by the new rule
            rule_vand(captured, parent_not_captured, tree->rule(i).truthtable, nsamples, &num_captured);
            // lower bound on antecedent support
            if ((tree->ablation() != 1) && (num_captured < threshold))
                continue;
            rule_vand(captured_zeros, captured, tree->label(0).truthtable, nsamples, &c0);
            c1 = num_captured - c0;
            if (c0 > c1) {
                prediction = 0;
                captured_correct = c0;
            } else {
                prediction = 1;
                captured_correct = c1;
            }
            // lower bound on accurate antecedent support
            if ((tree->ablation() != 1) && (captured_correct < threshold))
                continue;
            // subtract off parent equivalent points bound because we want to use pure lower bound from parent
            lower_bound = parent_lower_bound - parent_equivalent_minority + (double)(num_captured - captured_correct) / nsamples + c;
            logger->addToLowerBoundTime(time_diff(t1));
            logger->incLowerBoundNum();
            if (lower_bound >= tree->min_objective()) // hierarchical objective lower bound
                continue;
            double t2 = timestamp();
            rule_vandnot(not_captured, parent_not_captured, captured, nsamples, &num_not_captured);
            rule_vand(not_captured_zeros, not_captured, tree->label(0).truthtable, nsamples, &d0);
            d1 = num_not_captured - d0;
            if (d0 > d1) {
                default_prediction = 0;
                default_correct = d0;
            } else {
                default_prediction = 1;
                default_correct = d1;
            }
            objective = lower_bound + (double)(num_not_captured - default_correct) / nsamples;

            //printf("parent->objective(): %f\n", parent->objective());
            // if (tree->wpa())
            //     objective = parent->objective() - c1 * d0 + c * total_ones * total_zeros; 
            logger->addToObjTime(time_diff(t2));
            logger->incObjNum();
            // Should falling constraint go here too?
            double proportion = (double)c1/num_captured;
            double default_proportion = (double)d1/num_not_captured;
            if (objective < tree->min_objective() && random_search(random) && \
            (falling == false || has_falling_constraint(proportion, parent->proportion(), default_proportion))) {
                if (verbosity.count("progress")) {
                    printf("min(objective): %1.5f -> %1.5f, length: %d, cache size: %zu\n",
                    tree->min_objective(), objective, len_prefix, tree->num_nodes());
                }
                logger->setTreeMinObj(objective);
                tree->update_min_objective(objective);
                tree->update_opt_rulelist(parent_prefix, i);
                tree->update_opt_predictions(parent, prediction, default_prediction);
                // dump state when min objective is updated
                logger->dumpState();
            }
            // calculate equivalent points bound to capture the fact that the minority points can never be captured correctly
            if (tree->has_minority()) {
                rule_vand(not_captured_equivalent, not_captured, tree->minority(0).truthtable, nsamples, &num_not_captured_equivalent);
                equivalent_minority = (double)(num_not_captured_equivalent) / nsamples;
                lower_bound += equivalent_minority;
            }
            if (tree->ablation() != 2)
                lookahead_bound = lower_bound + c;
            else
                lookahead_bound = lower_bound;

            // Calculate lower bound using WPA
            if (tree->wpa()) {
                VECTOR parent_not_captured_zeroes;
                rule_vinit(nsamples, &parent_not_captured_zeroes);
                int num_parent_not_captured = count_ones_vector(parent_not_captured, nsamples);
                int r0;
                rule_vand(parent_not_captured_zeroes, parent_not_captured, tree->label(0).truthtable, nsamples, &r0);
                int r1 = num_parent_not_captured - r0;
                lookahead_bound = parent->objective() - r1 * r0 + c;
            }
            // only add node to our datastructures if its children will be viable
            // also add falling constraint
            if (lookahead_bound < tree->min_objective() && random_search(random) && \
                (falling == false || has_falling_constraint(proportion, parent->proportion(), default_proportion))) {
                double t3 = timestamp();
                // check permutation bound
                if (show_proportion)
                    printf("Proportion: %f\n", proportion);
                Node* n = p->insert(i, nrules, prediction, default_prediction,
                                    lower_bound, objective, parent, num_not_captured, nsamples,
                                    len_prefix, c, equivalent_minority, tree, not_captured, parent_prefix, proportion);
                logger->addToPermMapInsertionTime(time_diff(t3));
                // n is NULL if this rule fails the permutaiton bound
                if (n) {
                    double t4 = timestamp();
                    tree->insert(n);
                    logger->incTreeInsertionNum();
                    logger->incPrefixLen(len_prefix);
                    logger->addToTreeInsertionTime(time_diff(t4));
                    double t5 = timestamp();
                    q->push(n);
                    logger->setQueueSize(q->size());
                    if (tree->calculate_size())
                        logger->addQueueElement(len_prefix, lower_bound, false);
                    logger->addToQueueInsertionTime(time_diff(t5));
                }
            } // else:  objective lower bound with one-step lookahead
        }
    }
    /************************************** 
    *** Version to run when wpa is TRUE ***
    **************************************/
    else {
        double lb_array[nrules];
        for (i = 1; i < nrules; i++) {
            double t1 = timestamp();
            // check if this rule is already in the prefix
            if (std::find(parent_prefix.begin(), parent_prefix.end(), i) != parent_prefix.end())
                continue;
            // captured represents data captured by the new rule
            rule_vand(captured, parent_not_captured, tree->rule(i).truthtable, nsamples, &num_captured);
            // lower bound on antecedent support
            if ((tree->ablation() != 1) && (num_captured < threshold))
                continue;
            rule_vand(captured_zeros, captured, tree->label(0).truthtable, nsamples, &c0);
            c1 = num_captured - c0;
            if (c0 > c1) {
                prediction = 0;
                captured_correct = c0;
            } else {
                prediction = 1;
                captured_correct = c1;
            }
            // lower bound on accurate antecedent support
            if ((tree->ablation() != 1) && (captured_correct < threshold))
                continue;
            // subtract off parent equivalent points bound because we want to use pure lower bound from parent
            lower_bound = parent_lower_bound - parent_equivalent_minority + (double)(num_captured - captured_correct) / nsamples + c;
            logger->addToLowerBoundTime(time_diff(t1));
            logger->incLowerBoundNum();
            // if (lower_bound >= tree->min_objective()) // hierarchical objective lower bound
            //     continue;
            double t2 = timestamp();
            rule_vandnot(not_captured, parent_not_captured, captured, nsamples, &num_not_captured);
            rule_vand(not_captured_zeros, not_captured, tree->label(0).truthtable, nsamples, &d0);
            d1 = num_not_captured - d0;
            if (d0 > d1) {
                default_prediction = 0;
                default_correct = d0;
            } else {
                default_prediction = 1;
                default_correct = d1;
            }
            objective = lower_bound + (double)(num_not_captured - default_correct) / nsamples;

            //printf("parent->objective(): %f\n", parent->objective());

            if (tree->wpa()) {
                int support = tree->rule(i).support;
                objective = parent->objective() - c1 * d0 + c * total_ones * total_zeros;
                if (ties)
                    objective -= ties * 0.5 * (count_greater(captured, num_captured, tree->label(1).truthtable, nsamples) + count_greater(not_captured, num_not_captured, tree->label(1).truthtable, nsamples));
            }
            logger->addToObjTime(time_diff(t2));
            logger->incObjNum();
            // Should falling constraint go here too?
            double proportion = (double)c1/num_captured;
            double default_proportion = (double)d1/num_not_captured;
            if (objective < tree->min_objective() && \
            (falling == false || has_falling_constraint(proportion, parent->proportion(), default_proportion))) {
                if (verbosity.count("progress")) {
                    printf("min(objective): %1.5f -> %1.5f, length: %d, cache size: %zu\n",
                    tree->min_objective(), objective, len_prefix, tree->num_nodes());
                }
                logger->setTreeMinObj(objective);
                tree->update_min_objective(objective);
                tree->update_opt_rulelist(parent_prefix, i);
                tree->update_opt_predictions(parent, prediction, default_prediction);
                // dump state when min objective is updated
                logger->dumpState();
            }
            // calculate equivalent points bound to capture the fact that the minority points can never be captured correctly
            if (tree->has_minority()) {
                rule_vand(not_captured_equivalent, not_captured, tree->minority(0).truthtable, nsamples, &num_not_captured_equivalent);
                equivalent_minority = (double)(num_not_captured_equivalent) / nsamples;
                lower_bound += equivalent_minority;
            }
            if (tree->ablation() != 2)
                lookahead_bound = lower_bound + c;
            else
                lookahead_bound = lower_bound;

            // Calculate lower bound using WPA
            if (tree->wpa()) {
                VECTOR total_not_captured;
                rule_vinit(nsamples, &total_not_captured);
                int num_total_not_captured;
                VECTOR total_not_captured_zeroes, total_not_captured_ones;
                rule_vinit(nsamples, &total_not_captured_zeroes);
                rule_vinit(nsamples, &total_not_captured_ones);
                rule_vandnot(total_not_captured, parent_not_captured, captured, nsamples, &num_total_not_captured);
                // int num_parent_not_captured = count_ones_vector(parent_not_captured, nsamples);
                int r0, r1;
                rule_vand(total_not_captured_zeroes, total_not_captured, tree->label(0).truthtable, nsamples, &r0);
                rule_vand(total_not_captured_ones, total_not_captured, tree->label(1).truthtable, nsamples, &r1);
                // r1 = num_parent_not_captured - r0;
                int support = tree->rule(i).support;
                lookahead_bound = objective - r1 * r0 + c * total_zeros * total_ones;
                if (ties)
                    lookahead_bound -= ties * 0.5 * (count_greater(total_not_captured_ones, r1, tree->label(1).truthtable, nsamples) + count_greater(total_not_captured_zeroes, r0, tree->label(1).truthtable, nsamples));
            }
            lb_array[i] = lookahead_bound;
            // only add node to our datastructures if its children will be viable
            // also add falling constraint
            
            // if (lookahead_bound < tree->min_objective() && random_search(random) && \
            //     (falling == false || has_falling_constraint(proportion, parent->proportion(), default_proportion))) {
            //     double t3 = timestamp();
            //     // check permutation bound
            //     if (show_proportion)
            //         printf("Proportion: %f\n", proportion);
            //     if (ties)
            //         objective += ties * 0.5 * count_greater(not_captured, num_not_captured, tree->label(1).truthtable, nsamples);
            //     Node* n = p->insert(i, nrules, prediction, default_prediction,
            //                         lower_bound, objective, parent, num_not_captured, nsamples,
            //                         len_prefix, c, equivalent_minority, tree, not_captured, parent_prefix, proportion);
            //     logger->addToPermMapInsertionTime(time_diff(t3));
            //     // n is NULL if this rule fails the permutaiton bound
            //     if (n) {
            //         double t4 = timestamp();
            //         tree->insert(n);
            //         logger->incTreeInsertionNum();
            //         logger->incPrefixLen(len_prefix);
            //         logger->addToTreeInsertionTime(time_diff(t4));
            //         double t5 = timestamp();
            //         q->push(n);
            //         logger->setQueueSize(q->size());
            //         if (tree->calculate_size())
            //             logger->addQueueElement(len_prefix, lower_bound, false);
            //         logger->addToQueueInsertionTime(time_diff(t5));
            //     }
            // } // else:  objective lower bound with one-step lookahead
            
        }
        // fprintf(stderr, "\n\nUnsorted: ");
        // for (int i = 0; i < nrules; i++)
        //     fprintf(stderr, "%f ", lb_array[i]);
        // fprintf(stderr, "\n\nSorted: ");
        qsort(lb_array, nrules, sizeof(double), compare_doubles);
        // for (int i = 0; i < nrules; i++)
        //     fprintf(stderr, "%f ", lb_array[i]);

        int start = 0;
        for (int i = nrules - 1; i >= 0; i--) {
            if (lb_array[i] == 0) {
                start = i;
                break;
            }
        }
        // fprintf(stderr, "start: %d\n", start);
        /********************
        *** AFTER SORTING ***
        ********************/
        for (i = 1; i < nrules; i++) {
            double t1 = timestamp();
            // check if this rule is already in the prefix
            if (std::find(parent_prefix.begin(), parent_prefix.end(), i) != parent_prefix.end())
                continue;
            // captured represents data captured by the new rule
            rule_vand(captured, parent_not_captured, tree->rule(i).truthtable, nsamples, &num_captured);
            // lower bound on antecedent support
            if ((tree->ablation() != 1) && (num_captured < threshold))
                continue;
            rule_vand(captured_zeros, captured, tree->label(0).truthtable, nsamples, &c0);
            c1 = num_captured - c0;
            if (c0 > c1) {
                prediction = 0;
                captured_correct = c0;
            } else {
                prediction = 1;
                captured_correct = c1;
            }
            // lower bound on accurate antecedent support
            if ((tree->ablation() != 1) && (captured_correct < threshold))
                continue;
            // subtract off parent equivalent points bound because we want to use pure lower bound from parent
            lower_bound = parent_lower_bound - parent_equivalent_minority + (double)(num_captured - captured_correct) / nsamples + c;
            logger->addToLowerBoundTime(time_diff(t1));
            logger->incLowerBoundNum();
            // if (lower_bound >= tree->min_objective()) // hierarchical objective lower bound
            //     continue;
            double t2 = timestamp();
            rule_vandnot(not_captured, parent_not_captured, captured, nsamples, &num_not_captured);
            rule_vand(not_captured_zeros, not_captured, tree->label(0).truthtable, nsamples, &d0);
            d1 = num_not_captured - d0;
            if (d0 > d1) {
                default_prediction = 0;
                default_correct = d0;
            } else {
                default_prediction = 1;
                default_correct = d1;
            }
            objective = lower_bound + (double)(num_not_captured - default_correct) / nsamples;

            //printf("parent->objective(): %f\n", parent->objective());

            if (tree->wpa()) {
                int support = tree->rule(i).support;
                objective = parent->objective() - c1 * d0 + c * total_ones * total_zeros;
                if (ties)
                    objective -= ties * 0.5 * (count_greater(captured, num_captured, tree->label(1).truthtable, nsamples) + count_greater(not_captured, num_not_captured, tree->label(1).truthtable, nsamples));
            }
            logger->addToObjTime(time_diff(t2));
            logger->incObjNum();
            // Should falling constraint go here too?
            double proportion = (double)c1/num_captured;
            double default_proportion = (double)d1/num_not_captured;
            if (objective < tree->min_objective() && \
            (falling == false || has_falling_constraint(proportion, parent->proportion(), default_proportion))) {
                if (verbosity.count("progress")) {
                    printf("min(objective): %1.5f -> %1.5f, length: %d, cache size: %zu\n",
                    tree->min_objective(), objective, len_prefix, tree->num_nodes());
                }
                logger->setTreeMinObj(objective);
                tree->update_min_objective(objective);
                tree->update_opt_rulelist(parent_prefix, i);
                tree->update_opt_predictions(parent, prediction, default_prediction);
                // dump state when min objective is updated
                logger->dumpState();
            }
            // calculate equivalent points bound to capture the fact that the minority points can never be captured correctly
            if (tree->has_minority()) {
                rule_vand(not_captured_equivalent, not_captured, tree->minority(0).truthtable, nsamples, &num_not_captured_equivalent);
                equivalent_minority = (double)(num_not_captured_equivalent) / nsamples;
                lower_bound += equivalent_minority;
            }
            if (tree->ablation() != 2)
                lookahead_bound = lower_bound + c;
            else
                lookahead_bound = lower_bound;

            // Calculate lower bound using WPA
            if (tree->wpa()) {
                VECTOR total_not_captured;
                rule_vinit(nsamples, &total_not_captured);
                int num_total_not_captured;
                VECTOR total_not_captured_zeroes, total_not_captured_ones;
                rule_vinit(nsamples, &total_not_captured_zeroes);
                rule_vinit(nsamples, &total_not_captured_ones);
                rule_vandnot(total_not_captured, parent_not_captured, captured, nsamples, &num_total_not_captured);
                // int num_parent_not_captured = count_ones_vector(parent_not_captured, nsamples);
                int r0, r1;
                rule_vand(total_not_captured_zeroes, total_not_captured, tree->label(0).truthtable, nsamples, &r0);
                rule_vand(total_not_captured_ones, total_not_captured, tree->label(1).truthtable, nsamples, &r1);
                // r1 = num_parent_not_captured - r0;
                int support = tree->rule(i).support;
                lookahead_bound = objective - r1 * r0 + c * total_zeros * total_ones;
                if (ties)
                    lookahead_bound -= ties * 0.5 * (count_greater(total_not_captured_ones, r1, tree->label(1).truthtable, nsamples) + count_greater(total_not_captured_zeroes, r0, tree->label(1).truthtable, nsamples));
                lower_bound = lookahead_bound;
            }
            // lb_array[i] = lookahead_bound;
            // only add node to our datastructures if its children will be viable
            // also add falling constraint
            
            if (lookahead_bound < tree->min_objective() && random_search(random) && \
            lookahead_bound < lb_array[(int)(start + bound * (nrules - 1 - start))] && \
                (falling == false || has_falling_constraint(proportion, parent->proportion(), default_proportion))) {
                double t3 = timestamp();
                // check permutation bound
                if (show_proportion)
                    printf("Proportion: %f\n", proportion);
                if (ties)
                    objective += ties * 0.5 * count_greater(not_captured, num_not_captured, tree->label(1).truthtable, nsamples);
                Node* n = p->insert(i, nrules, prediction, default_prediction,
                                    lower_bound, objective, parent, num_not_captured, nsamples,
                                    len_prefix, c, equivalent_minority, tree, not_captured, parent_prefix, proportion);
                logger->addToPermMapInsertionTime(time_diff(t3));
                // n is NULL if this rule fails the permutaiton bound
                if (n) {
                    double t4 = timestamp();
                    tree->insert(n);
                    logger->incTreeInsertionNum();
                    logger->incPrefixLen(len_prefix);
                    logger->addToTreeInsertionTime(time_diff(t4));
                    double t5 = timestamp();
                    q->push(n);
                    logger->setQueueSize(q->size());
                    if (tree->calculate_size())
                        logger->addQueueElement(len_prefix, lower_bound, false);
                    logger->addToQueueInsertionTime(time_diff(t5));
                }
            } // else:  objective lower bound with one-step lookahead
            
        }
    }
    
    rule_vfree(&captured);
    rule_vfree(&captured_zeros);
    rule_vfree(&not_captured);
    rule_vfree(&not_captured_zeros);
    rule_vfree(&not_captured_equivalent);
    
    logger->addToRuleEvalTime(time_diff(t0));
    logger->incRuleEvalNum();
    logger->decPrefixLen(parent->depth());
    if (tree->calculate_size())
        logger->removeQueueElement(len_prefix - 1, parent_lower_bound, false);
    if (parent->num_children() == 0) {
        tree->prune_up(parent);
    } else {
        parent->set_done();
        tree->increment_num_evaluated();
    }
}

/*
 * Explores the search space by using a queue to order the search process.
 * The queue can be ordered by DFS, BFS, or an alternative priority metric (e.g. lower bound).
 */
int bbound(CacheTree* tree, size_t max_num_nodes, Queue* q, PermutationMap* p)
{
    return bbound(tree, max_num_nodes, q, p, false, false, false, 0, 1, 1);
}

int bbound(CacheTree* tree, size_t max_num_nodes, Queue* q, PermutationMap* p, bool falling, bool show_proportion, bool change_search_path, double ties, double random, double bound) {
    size_t num_iter = 0;
    int cnt;
    double min_objective;
    VECTOR captured, not_captured;
    rule_vinit(tree->nsamples(), &captured);
    rule_vinit(tree->nsamples(), &not_captured);
    
    size_t queue_min_length = logger->getQueueMinLen();
    
    double start = timestamp();
    logger->setInitialTime(start);
    logger->initializeState(tree->calculate_size());
    int verbosity = logger->getVerbosity();
    // initial log record
    logger->dumpState();
    
    min_objective = 1.0;
    tree->insert_root();
    logger->incTreeInsertionNum();
    q->push(tree->root());
    logger->setQueueSize(q->size());
    logger->incPrefixLen(0);
    // log record for empty rule list
    logger->dumpState();
 //   if (tree->wpa())
 //       min_objective = tree->min_objective();
    while ((tree->num_nodes() < max_num_nodes) && !q->empty()) {
        double t0 = timestamp();
        std::pair<Node*, tracking_vector<unsigned short, DataStruct::Tree> > node_ordered = q->select(tree, captured);
        logger->addToNodeSelectTime(time_diff(t0));
        logger->incNodeSelectNum();
        //printf("node_ordered.first: %f\n", node_ordered.first);
        if (node_ordered.first) {
            //printf("Calling evaluate_children()\n");
            double t1 = timestamp();
            // not_captured = default rule truthtable & ~ captured
            rule_vandnot(not_captured,
                         tree->rule(0).truthtable, captured,
                         tree->nsamples(), &cnt);
            evaluate_children(tree, node_ordered.first, node_ordered.second, not_captured, q, p, falling, show_proportion, change_search_path, ties, random, bound);
            logger->addToEvalChildrenTime(time_diff(t1));
            logger->incEvalChildrenNum();
            
            if (tree->min_objective() < min_objective) {
                min_objective = tree->min_objective();
                if (verbosity >= 10)
                    printf("before garbage_collect. num_nodes: %zu, log10(remaining): %zu\n", 
                            tree->num_nodes(), logger->getLogRemainingSpaceSize());
                logger->dumpState();
                tree->garbage_collect();
                logger->dumpState();
                if (verbosity >= 10)
                    printf("after garbage_collect. num_nodes: %zu, log10(remaining): %zu\n", tree->num_nodes(), logger->getLogRemainingSpaceSize());
            }
        }
        logger->setQueueSize(q->size());
        if (queue_min_length < logger->getQueueMinLen()) {
            // garbage collect the permutation map: can be simplified for the case of BFS
            queue_min_length = logger->getQueueMinLen();
            //pmap_garbage_collect(p, queue_min_length);
        }
        ++num_iter;
        if ((num_iter % 10000) == 0) {
            if (verbosity >= 10)
                printf("iter: %zu, tree: %zu, queue: %zu, pmap: %zu, log10(remaining): %zu, time elapsed: %f\n",
                       num_iter, tree->num_nodes(), q->size(), p->size(), logger->getLogRemainingSpaceSize(), time_diff(start));
        }
        if ((num_iter % logger->getFrequency()) == 0) {
            // want ~1000 records for detailed figures
            logger->dumpState();
        }
    }
    logger->dumpState(); // second last log record (before queue elements deleted)
    if (verbosity >= 1)
        printf("iter: %zu, tree: %zu, queue: %zu, pmap: %zu, log10(remaining): %zu, time elapsed: %f\n",
               num_iter, tree->num_nodes(), q->size(), p->size(), logger->getLogRemainingSpaceSize(), time_diff(start));
    if (q->empty())
        printf("Exited because queue empty\n");
    else
        printf("Exited because max number of nodes in the tree was reached\n");

    size_t tree_mem = logger->getTreeMemory(); 
    size_t pmap_mem = logger->getPmapMemory(); 
    size_t queue_mem = logger->getQueueMemory(); 
    printf("TREE mem usage: %zu\n", tree_mem);
    printf("PMAP mem usage: %zu\n", pmap_mem);
    printf("QUEUE mem usage: %zu\n", queue_mem);

    // Print out queue
    ofstream f;
    if (print_queue) {
        char fname[] = "queue.txt";
        printf("Writing queue elements to: %s\n", fname);
        f.open(fname, ios::out | ios::trunc);
        f << "lower_bound objective length frac_captured rule_list\n";
    }
    
    // Clean up data structures
    if (verbosity.count("progress")) {
        printf("Deleting queue elements and corresponding nodes in the cache,"
                "since they may not be reachable by the tree's destructor\n");
        printf("\nminimum objective: %1.10f\n", tree->min_objective());
    }

    // DEBIGGING ONLY (disable logging)
    return num_iter;

    Node* node;
    double min_lower_bound = 1.0;
    double lb;
    size_t num = 0;
    while (!q->empty()) {
        node = q->front();
        q->pop();
        if (node->deleted()) {
            tree->decrement_num_nodes();
            logger->removeFromMemory(sizeof(*node), DataStruct::Tree);
            delete node;
        } else {
            lb = node->lower_bound() + tree->c();
            if (lb < min_lower_bound)
                min_lower_bound = lb;
            if (print_queue) {
                std::pair<tracking_vector<unsigned short, DataStruct::Tree>, tracking_vector<bool, DataStruct::Tree> > pp_pair = node->get_prefix_and_predictions();
                tracking_vector<unsigned short, DataStruct::Tree> prefix = std::move(pp_pair.first);
                tracking_vector<bool, DataStruct::Tree> predictions = std::move(pp_pair.second);
                f << node->lower_bound() << " " << node->objective() << " " << node->depth() << " "
                << (double) node->num_captured() / (double) tree->nsamples() << " ";
                for(size_t i = 0; i < prefix.size(); ++i) {
                    f << tree->rule_features(prefix[i]) << "~"
                    << predictions[i] << ";";
                }
                f << "default~" << predictions.back() << "\n";
                num++;
            }
        }
    }
    printf("minimum lower bound in queue: %1.10f\n\n", min_lower_bound);
    if (print_queue)
        f.close();
    // last log record (before cache deleted)
    logger->dumpState();
    
    rule_vfree(&captured);
    rule_vfree(&not_captured);
    return num_iter;
}
