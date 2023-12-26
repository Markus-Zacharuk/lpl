#include "Header.h"


namespace Eval {
    
    

    auto operator<< (std::ostream& o, const Rule& rule) -> std::ostream& {
        o << "rule (";
        std::string sep = "";
        for (const auto& arg: rule.arguments) {
            o << sep << arg;
            sep = ", ";
        }
        
        o << ") => ";
        sep = "";
        for (const auto& transformat: rule.transformats) {
            o << sep << transformat;
            sep = ", ";
        }
        o << ";";
        return o;
    }

    auto operator<< (std::ostream& o, const Axiom& axiom) -> std::ostream& {
        o << "axiom (";
        std::string sep = "";
        for (const auto& arg: axiom.arguments) {
            o << sep << arg;
            sep = ", ";
        }
        
        o << ") => ";
        sep = "";
        for (const auto& transformat: axiom.transformats) {
            o << sep << transformat;
            sep = ", ";
        }
        o << ";";
        return o;
    }

    auto operator<< (std::ostream& o, const local_names_map_t& names) -> std::ostream& {
        o << "local_names_map_t = {\n";
        std::string sep = "    ";
        for (const auto& [key, value]: names)
            o << sep << "{" << key << ", " << value << "}";
        //    sep = ",\n    ";
        o << "\n}\n";
        return o;
    }

    auto operator<< (std::ostream& o, const name_value_t& value) -> std::ostream& {
        boost::apply_visitor(boost::hana::overload(
                [&](const auto& arg) -> void { o << arg; }
            ), value);
        return o;
    }

    auto operator<< (std::ostream& o, const names_map_t& names) -> std::ostream& {
        o << "names_map_t = {\n";
        std::string sep = "    ";
        for (const auto& [key, value]: names)
            o << sep << "{" << key << ", " << value << "}";
        //    sep = ",\n    ";
        o << "\n}\n";
        return o;
    }

    auto eval(const AST::AxiomBlock& axiom, Context& context) -> bool {
        if (context.names.contains(axiom.name)) {
            if (context.warn_redefinition)
                DBGQ("Warning", "Redefinition", "Name: " + axiom.name + ", Hint: Redefinition not applied.", axiom);
            return true;
        }
        //DBG("Debug", "Checking definition of axiom", axiom.name);
        context.names[axiom.name] = Axiom{ axiom.transformats, axiom.arguments };
        //std::cout << context.names;
        return true;
    }

    auto verify(const AST::Hypothesis& hypothesis, const Context& context, local_names_map_t& names) -> bool {
        // First add the actual hypothesis to the names. It will be removed on exiting the function.
        if (names.contains(hypothesis.condition_name)) {
            if (context.warn_redefinition)
                DBGQ("Warning", "Redefinition", "Name: " + hypothesis.condition_name + ", Hint: Redefinition not applied.", hypothesis);
            return false;
        }
        names[hypothesis.condition_name] = hypothesis.condtition;

        std::unordered_set<std::string> names_in_hypothesis_scope;
        bool success = true;
        for (const auto& statement: hypothesis.statements) {
            success = boost::apply_visitor(boost::hana::overload(
                [&](const AST::Definition& definition) -> bool {
                    names_in_hypothesis_scope.insert(definition.name);
                    return verify(definition, context, names);
                },
                [&](const AST::Hypothesis& hypothesis) -> bool {
                    return verify(hypothesis, context, names);
                }
            ), statement);
            if (!success) break;
        }
        for (const auto& name: names_in_hypothesis_scope) 
            names[name] = AST::BinOp(AST::BinOpID::Implication, hypothesis.condtition, names[name]);
        names.erase(hypothesis.condition_name);
        return success;
    }

    auto resolve_global_name(const std::string& name, const Context& context) -> const Eval::name_value_t* {
        Eval::names_map_t::const_iterator context_search_result = context.names.find(name);
        if (context_search_result != context.names.end())
            return &(context_search_result->second);
        else return nullptr;
    }

    template <typename T>
    auto  resolve_global_typed(const std::string& name, const Context& context) -> const T* {
        //std::cout << __PRETTY_FUNCTION__ << "\n";
        auto search_result = resolve_global_name(name, context);
        //std::cout << "&search_result = " << search_result << "\n";
        if (!search_result) return nullptr;
        return  boost::get<T>(search_result);
    }

    auto resolve_local_name(const std::string& name, const local_names_map_t& names) -> const AST::lformula_t* {
        Eval::local_names_map_t::const_iterator context_search_result = names.find(name);
        if (context_search_result != names.end()) 
            return &(context_search_result->second);
        return nullptr;
    }



    auto create_name_pattern(
        const AST::lformula_t& to, const AST::lformula_t& from, const local_names_map_t& names, local_names_map_t& name_pattern,
        bool to_is_name_resolved=false) 
        -> bool 
    {
        return boost::apply_visitor(boost::hana::overload(
                [&](const std::string& to_, const std::string& from_) -> bool { 
                    if (to_is_name_resolved) {
                        if (to_ == from_) return true;
                        const auto search_result_from_ = names.find(from_);
                        if (search_result_from_ != names.end())
                            return create_name_pattern(to, search_result_from_->second, names, name_pattern, true);
                        std::stringstream msg;
                        msg << "Could not resolve " << from << " to " << to << ".";
                        //DBG("Error", "Evaluation", msg.str());
                        return false;
                    }
                    const auto search_result = name_pattern.find(to_);
                    if (search_result != name_pattern.end())
                        return create_name_pattern(search_result->second, from, names, name_pattern, true);              
                    name_pattern[to_] = from;
                    return true;
                },

                [&](const AST::UnOp& to_, const std::string& from_) -> bool {
                    const auto search_result = resolve_local_name(from_, names);
                    if (search_result == nullptr) return false;
                    return create_name_pattern(to_, *search_result, names, name_pattern, to_is_name_resolved);
                 },
                [&](const std::string& to_, const AST::UnOp& from_) -> bool { 
                    if (to_is_name_resolved) return false;
                    const auto search_result = name_pattern.find(to_);
                    if (search_result != name_pattern.end())
                        return create_name_pattern(search_result->second, from, names, name_pattern, true);
                    name_pattern[to_] = from;
                    return true;
                 },
                [&](const AST::UnOp& to_, const AST::UnOp& from_) -> bool { 
                    if (to_.id != from_.id) return false;
                    return create_name_pattern(to_.rhs, from_.rhs, names, name_pattern, to_is_name_resolved);
                },
                
                [&](const AST::BinOp& to_, const std::string& from_) -> bool { 
                    const auto search_result = resolve_local_name(from_, names);
                    if (search_result == nullptr) return false;
                    return create_name_pattern(to_, *search_result, names, name_pattern, to_is_name_resolved);
                 },
                [&](const std::string& to_, const AST::BinOp& from_) -> bool { 
                    if (to_is_name_resolved) return false;
                    const auto search_result = name_pattern.find(to_);
                    if (search_result != name_pattern.end())
                        return create_name_pattern(search_result->second, from, names, name_pattern, true);
                    name_pattern[to_] = from;
                    return true;
                 },
                [&](const AST::BinOp& to_, const AST::BinOp& from_) -> bool {
                    if (to_.id != from_.id) return false;
                    bool result_lhs = create_name_pattern(to_.lhs, from_.lhs, names, name_pattern, to_is_name_resolved);
                    bool result_rhs = create_name_pattern(to_.rhs, from_.rhs, names, name_pattern, to_is_name_resolved);
                    if (result_lhs && result_rhs) return true;

                    std::stringstream msg;
                    msg << "Could not resolve " << from << " to " << to << ". " << to_is_name_resolved;
                    //DBG("Error", "Evaluation", msg.str());
                    return false; 
                },

                // cases (UnOp, BinOp), (BinOp, UnOp) where the Operations don't match.
                [&](const auto& to_, const auto& from_) -> bool { 
                    std::stringstream msg;
                    msg << "Could not transform " << from << " to " << to << ", because they have different types.";
                    //DBG("Error", "Evaluation", msg.str());
                    return false; 
                    }
                ), to, from);
    }


    auto verify_by_transformation(
        const Axiom& axiom, const std::vector<std::string>& arguments, const AST::lformula_t& result, 
        const Context& context, local_names_map_t& names) 
        -> bool
    {
        const size_t argument_count = axiom.arguments.size();
        if (argument_count != arguments.size()) return false;

        local_names_map_t name_pattern;
        for (size_t i = 0; i < argument_count; ++i) {
            auto search_result = resolve_local_name(arguments[i], names);
            if (!search_result) {
                DBGQ("Error", "Undefined Symbol", "Name: " + arguments[i] + ". Happend in proof of the following:", result);
                return false;
            }
            if (!create_name_pattern(axiom.arguments[i], *search_result, names, name_pattern)) {
                DBG("Error", "Evaluation", "Could not resolve symbols to proof. (argument " + std::to_string(i) + "=" + arguments[i] + ")" );
                return false;
            }
        }
        for (size_t i = 0; i < axiom.transformats.size(); ++i) {
            auto name_pattern_copy = name_pattern;
            if (create_name_pattern(axiom.transformats[i], result, names, name_pattern_copy))
                return true;
        }
        std::stringstream s; s << name_pattern;
        DBG("Error", "Evaluation", "Could not transform transformat to proof. name_pattern: " + s.str());
        
        return false;
    }

    auto verify_by_transformation(
        const Rule& rule, const std::vector<std::string>& arguments, const AST::lformula_t& result, 
        const Context& context, local_names_map_t& names) 
        -> bool
    {
        return verify_by_transformation(Eval::Axiom{rule.transformats, rule.arguments}, arguments, result, context, names);
    }

    auto verify(const AST::Definition& definition, const Context& context, local_names_map_t& names) -> bool {
        if (context.names.contains(definition.name) || names.contains(definition.name)) {
            if (context.warn_redefinition)
                DBGQ("Warning", "Redefinition", "Name: " + definition.name + ", Hint: Redefinition not applied.", definition);
            return true;
        }

        const auto search_result = resolve_global_name(definition.reasoning.name, context);
        if (!search_result) {
            DBGQ("Error", "Undefined Symbol", "Name: " + definition.reasoning.name, definition);
            return false;
        }
        bool proof_is_correct = boost::apply_visitor(boost::hana::overload(
            [&](const Rule& rule) -> bool { 
                return verify_by_transformation(rule, definition.reasoning.arguments, definition.value, context, names); 
                },
            [&](const Axiom& axiom) -> bool { 
                return verify_by_transformation(axiom, definition.reasoning.arguments, definition.value, context, names); 
                }
            ), *search_result);

        if (!proof_is_correct) { 
            DBGQ("Error", "Evaluation", "Could not verify that " + definition.reasoning.name + " is proof: ", definition);            
            return false;
        }

        names[definition.name] = definition.value;
        return true;
    }

    auto verify(const AST::RuleBlock& rule, const Context& context) -> bool {
        if (rule.statements.empty()) {
            DBGQ("Error", "Evalutation", "Rule " + rule.name +  " has empty body.", rule);
            return false;
        }
        if (rule.transformats.size() > rule.statements.size()) { 
            std::string r = std::to_string(rule.statements.size());
            std::string s = std::to_string(rule.transformats.size());
            DBGQ("Error", "Evalutation", "Rule " + rule.name +  " has more results (" + r + ") than statements (" + s +").", rule);
            return false;
        }

        local_names_map_t names;

        for (size_t i = 0; i < rule.arguments.size(); ++i)
            names["arg" + std::to_string(i+1)] = rule.arguments[i];

        for (const auto& statement: rule.statements) {
            bool success = boost::apply_visitor(boost::hana::overload(
                [&](const auto& arg) -> bool { return verify(arg, context, names); } 
                ), statement);
            if (!success) {
                DBGQ("Error", "Evaluation", "Could not verify step in rule: " + rule.name + ". The step:", statement);
                return false;
            }
        }

        // Check for QED.
        size_t j = 0;
        for (size_t i = rule.statements.size() - rule.transformats.size(); i < rule.statements.size(); ++i) {
            //std::cout << i << " of " << rule.statements.size() << " :: " << rule.statements.size() << std::endl;

            bool success = boost::apply_visitor(boost::hana::overload(
                [&](const AST::Definition& def) -> bool { 
                    return AST::equal_symbolic(def.value, rule.transformats[0]); 
                },
                [&](const auto&) -> bool { return false; }
            ), rule.statements[i]);
            if (!success) {
                DBGQ("Error", "Evaluation", "Could not verify conclusion from last step of rule: " + rule.name + ".", rule.statements[i]);
                return false;
            }
             ++j;
        }
        return true;
    }

    auto eval(const AST::RuleBlock& rule, Context& context) -> bool {
        if (context.names.contains(rule.name)) {
            if (context.warn_redefinition)
                DBGQ("Warning", "Redefinition", "Name: " + rule.name + ", Hint: Redefinition not applied.", rule);
            return true;
        }
        if (verify(rule, context)) {
            context.names[rule.name] = Rule{ rule.transformats, rule.arguments };
            return true;
        }
        DBGQ("Error", "Evaluation", "Could not verify Rule: " + rule.name, rule);
        return false;
    }

    auto eval(const AST::block_t& block, Context& context) -> bool {
        return boost::apply_visitor(boost::hana::overload(
            [&](const auto& block_) -> bool { return eval(block_, context); }
        ), block);
    }

    auto eval(const AST::file_t& file, Context& context) -> bool{
        for (const auto& block: file) {
            //std::cout << "Evaluating: " << block << std::endl << std::endl;
            if (!eval(block, context)) return false;
        }
        return true;
    }
}
