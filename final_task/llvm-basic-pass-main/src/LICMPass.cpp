/**
 * Invariant Code Motion algorithm
 */
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include "llvm/ADT/PostOrderIterator.h"

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"

#include "llvm/Support/raw_ostream.h"

#include <map>
#include <set>
#include <string>
#include <stack>
#include <vector>
#include <algorithm>

#define uns unsigned

using namespace llvm;


namespace {


// ----------------------------------------------------------------
// Pre-order Traversal Pass
// includes saving last node's descendants

struct ExtendedPreOrder {
    std::vector<BasicBlock*> node;      // RPO[id] = ptr
    std::vector<unsigned> last;         // last[par_id] = max_succ_id
    std::map<BasicBlock*, uns> number;  // number[elem] = id
};

class ExtendedPreOrderPass : public AnalysisInfoMixin<ExtendedPreOrderPass> {
    friend AnalysisInfoMixin<ExtendedPreOrderPass>;
    static inline AnalysisKey Key;
public:
    using Result = ExtendedPreOrder;

    Result run(Function& Function, FunctionAnalysisManager& AM) {
        std::vector<uns> last(Function.size(), 0);
        std::vector<BasicBlock*> node(Function.size(), nullptr); // node[id] = elem
        std::map<BasicBlock*, uns> number; // number[elem] = id
        
        uns current = 0;
        
        std::vector<bool> discovered(Function.size(), false);
        std::stack<BasicBlock*> workStack;

        workStack.push(&Function.getEntryBlock());
        while (!workStack.empty()) {
            auto curr = workStack.top();

            number[curr] = current;
            node[current++] = curr;

            if (!discovered[curr->getNumber()]) {
                discovered[curr->getNumber()] = true;

                auto currT = curr->getTerminator();
                for (uns i = 0, n = currT->getNumSuccessors(); i < n; i++) {
                    if (!discovered[currT->getSuccessor(i)->getNumber()]) {
                        workStack.push(currT->getSuccessor(i));
                    }
                }
                
                continue;
            }

            workStack.pop();
            last[number[curr]] = current - 1;
        }

        // shrinking return vectors
        uns sumB = 0;
        for (auto iter = discovered.rbegin(); iter != discovered.rend(); ++iter) {
            if (!*iter) {
                last.pop_back(); node.pop_back(); // if not discovered, it's not been put in these
            }
        }
        last.shrink_to_fit(); node.shrink_to_fit();

        return {node, last, number};
    }
};



// ----------------------------------------------------------------
// Pass to build Loop Nesting Tree

enum BBlock_type {
    nonheader, reducible, irred, self
};

struct LoopNestingTree {
    ExtendedPreOrder PreOrder;
    std::vector<int> headers;
    std::vector<BBlock_type> types;
};

template<typename T>
bool isVectorMember(std::vector<T>& vec, const T& val) {
    for (auto& i : vec) {
        if (i == val) {
            return true;
        }
    }
    return false;
}

// separate analysis pass
class LoopNestingTreePass : public AnalysisInfoMixin<LoopNestingTreePass> {
    friend AnalysisInfoMixin<LoopNestingTreePass>;
    static inline AnalysisKey Key;
public:
    using Result = LoopNestingTree;
    
    // Algorithm follows Paul Havlak's nesting loops algorithm
    // Original denotement is saved accordingly 
    Result run(Function& Function, FunctionAnalysisManager& AM) {
        auto preOrderRes = AM.getResult<ExtendedPreOrderPass>(Function);

        // header[block] = header of the smallest loop block is in
        std::vector<int> header(preOrderRes.last.size(), 0); // default header
        std::vector<BBlock_type> type(preOrderRes.last.size(), BBlock_type::nonheader);

        std::vector<std::vector<uns>> nonBackPreds(preOrderRes.last.size());
        std::vector<std::vector<uns>> backPreds(preOrderRes.last.size());

        std::vector<std::set<uns>> unions(preOrderRes.last.size());
        for (uns i = 0; i < preOrderRes.last.size(); ++i) {
            unions[i].emplace(i);
        }

        auto FIND = [&unions](uns v) {
            for (uns i = 0; i < unions.size(); ++i) {
                if (unions[i].find(v) != unions[i].end()) {
                    return (int)i;
                }
            }

            return -1; // must not happen
        };

        auto UNION = [&unions](uns v, uns w) -> void {
            unions[w].merge(unions[v]);
        };

        auto isAncestor = [&preOrderRes](uns v, uns w) {
            return (w <= v) && (v <= preOrderRes.last[w]);
        };
        

        for (uns w = 0; w < preOrderRes.last.size(); ++w) {
            auto currBB = preOrderRes.node[w];
            for ( auto it = pred_begin(currBB), et = pred_end(currBB); it != et; ++it) {
                auto v = preOrderRes.number[*it];
                if (isAncestor(w, v)) {
                    backPreds[w].push_back(v);
                } else {
                    nonBackPreds[w].push_back(v);
                }
            } 
        }

        header[0] = -1;

        for (int w = preOrderRes.last.size() - 1; w >= 0; --w) {
            std::vector<uns> P;

            for (auto v : backPreds[w]) {
                if (v != w) {
                    P.push_back(FIND(v));
                } else {
                    type[w] = BBlock_type::self;
                }
            }

            std::vector<uns> workList = P;
            if (!P.empty()) {
                type[w] = BBlock_type::reducible;
            }
            
            while (!workList.empty()) {
                auto x = workList[workList.size() - 1];
                workList.pop_back();

                for (auto y : nonBackPreds[x]) {
                    auto y_ = (uns)FIND(y);
                    if (!isAncestor(w, y_)) {
                        type[w] = BBlock_type::irred;
                        nonBackPreds[w].push_back(y_);
                    } else if (!isVectorMember(P, y_) && y_ != w) {
                        P.push_back(y_);
                        workList.push_back(y_);
                    }
                }
            }

            for (auto x : P) {
                header[x] = w;
                UNION(x, w);
            }

        }


        return {preOrderRes, header, type};
    }
};





// ----------------------------------------------------------------
// Main pass -- Loop Invariant Code Motion


struct LICMPass : PassInfoMixin<LICMPass> 
{
    bool isLoopMember(LoopNestingTree& LoopNesting, uns targetLoop, uns BB) {
        uns headIter = LoopNesting.headers[BB];
        while (headIter != 0) {
            if (headIter == targetLoop) {
                return true;
            }

            headIter = LoopNesting.headers[headIter];
        }

        return false;
    }

    BasicBlock* insertBBBetween(std::vector<BasicBlock*> predsBB, BasicBlock* succBB) {
        auto newBB = BasicBlock::Create(succBB->getContext(), "", succBB->getParent());
        newBB->moveAfter(predsBB[0]);

        for (auto predBB : predsBB) {
            auto terminator = predBB->getTerminator();
            terminator->replaceUsesOfWith(succBB, newBB);
        }

        IRBuilder<> Builder(newBB);
        Builder.CreateBr(succBB);

        return newBB;
    }

    #define insertPreheader insertBBBetween

    void moveToPreheader(LoopNestingTree& LoopNesting, Instruction* targetInst, uns targetLoop) {
        auto Loop = LoopNesting.PreOrder.node[targetLoop];
        
        std::vector<BasicBlock*> preheaderCandidates;
        for (auto pred : predecessors(Loop)) {
            uns predPO = LoopNesting.PreOrder.number[pred];
            if (!isLoopMember(LoopNesting, targetLoop, predPO)) {
                preheaderCandidates.push_back(pred);
            }
        }

        // if unique non-loop pred, it's preheader
        if (preheaderCandidates.size() == 1) {
            targetInst->removeFromParent();
            targetInst->insertInto(preheaderCandidates[0], preheaderCandidates[0]->begin());
        }

        // preheader insertion needed
        else if (preheaderCandidates.size() > 1) {
            auto preheader = insertPreheader(preheaderCandidates, Loop);
            targetInst->removeFromParent();
            targetInst->insertInto(preheaderCandidates[0], preheaderCandidates[0]->begin());
        }

        else errs() << "zero non-loop predecessors found!\n";
    }


    PreservedAnalyses 
    run(Function &Function, 
	    FunctionAnalysisManager &AM) 
    {
        LoopNestingTree LoopNesting = AM.getResult<LoopNestingTreePass>(Function);

        

        // iterate in reverse pre-order
        for (uns BBlockPOrder = LoopNesting.headers.size() - 1; BBlockPOrder >= 0; --BBlockPOrder) {
            // headers that suit algorithm
            if (LoopNesting.types[BBlockPOrder] == BBlock_type::reducible || 
                LoopNesting.types[BBlockPOrder] == BBlock_type::self) {

                    bool Changed = true;
                    while (Changed) {
                        for (uns DescendantPO = BBlockPOrder; DescendantPO <= LoopNesting.PreOrder.last[BBlockPOrder]; ++DescendantPO) {
                            if (LoopNesting.headers[DescendantPO] == BBlockPOrder) {
                                for (auto& Def : *(LoopNesting.PreOrder.node[DescendantPO])) {
                                    if (llvm::isa<llvm::LoadInst>(Def)) {
                                        continue;
                                    }

                                    bool Move = true;
                                    for (uns i = 0; i < Def.getNumOperands(); ++i) {
                                        auto operand = Def.getOperand(i);
                                        auto operandBB = dyn_cast<Instruction>(operand)->getParent();
                                        uns operandBBPO = LoopNesting.PreOrder.number[operandBB];

                                        if (isLoopMember(LoopNesting, BBlockPOrder, operandBBPO)) {
                                            Move = false; break;
                                        }
                                    }

                                    if (!Move) continue;
                                    moveToPreheader(LoopNesting, &Def, BBlockPOrder);
                                    Changed = true;
                                }
                            }
                        }
                    }
                }
            

        }


        return (PreservedAnalyses::none());
    }

    static bool 
    isRequired( void) 
    { 
        return (true); 
    }
};

} /* namespace */

/**
 * Наш проход будет реализован в виде отдельно подключаемого плагина (расширения языка).
 * Это удобный способ расширить возможности компилятора. Например, сделать поддержку
 * своей прагмы, своей оптимизации, выдачи своего предупреждения.
 *
 * PassPluginLibraryInfo - структура, задающая базовые параметры для нашего плагина.
 * Её надо составить из:
 * - Версии API (для отслеживания совместимости ABI можно 
 *   использовать LLVM_PLUGIN_API_VERSION)
 * - Имя плагина
 * - Версии плагина
 * - Callback для регистрации плагина через PassBuilder
 */
/**
 * По-модному это делают с помощью лямбда-функции, но можно и по-старинке.
 */
bool
CallBackForPipelineParser( 
    StringRef Name, 
    FunctionPassManager &FPM,  
    ArrayRef<PassBuilder::PipelineElement>)
{
    if ( Name == "LICMPass" )
    {
        FPM.addPass( LICMPass());
	return (true);
    } else
    {
        return (false);
    }
} /* CallBackForPipelineParser */

void
CallBackForAnalysisRegistration(FunctionAnalysisManager &FAM) {
    // Register analysis
    FAM.registerPass([&] { return LoopNestingTreePass(); });
    FAM.registerPass([&] { return ExtendedPreOrderPass(); });
}


void
CallBackForPassBuilder( PassBuilder &PB)
{
    PB.registerPipelineParsingCallback( &CallBackForPipelineParser); 

    PB.registerAnalysisRegistrationCallback(&CallBackForAnalysisRegistration);
} /* CallBackForPassBuider */

PassPluginLibraryInfo 
getPluginInfo( void)
{
    uint32_t     APIversion =  LLVM_PLUGIN_API_VERSION;
    const char * PluginName =  "LICMPass";
    const char * PluginVersion =  LLVM_VERSION_STRING;
    
    PassPluginLibraryInfo info = 
    { 
        APIversion, 
	    PluginName, 
	    PluginVersion, 
	    CallBackForPassBuilder
    };

  return (info);
} /* getMyPassPluginInfo */

/**
 * Интерфейс, который гарантирует, что "opt" распознаст наш проход. 
 * "-passes=MyPass"
 */
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() 
{
  return (getPluginInfo());
} /* llvmGetPassPluginInfo */
