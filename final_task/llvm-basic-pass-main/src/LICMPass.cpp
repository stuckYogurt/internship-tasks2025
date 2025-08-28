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
        errs() << "PreOrder DFST on " << Function.getName();

        std::vector<uns> last(Function.size(), 0);
        std::vector<BasicBlock*> node(Function.size(), nullptr); // node[id] = elem
        std::map<BasicBlock*, uns> number; // number[elem] = id
        
        uns current = 0;
        
        std::vector<bool> discovered(Function.size(), false);
        std::stack<BasicBlock*> workStack;

        workStack.push(&Function.getEntryBlock());

        while (!workStack.empty()) {
            auto curr = workStack.top();
            
            if (!discovered[curr->getNumber()]) {
                number[curr] = current;
                node[current++] = curr;
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

        // errs() << "discovers!\n";
        // for (auto i : discovered) {
        //     errs() << i << " ";
        // }

        // errs() << "lasters\n";
        // for (auto i : last) {
        //     errs() << i << " ";
        // }

        // errs() << "noders\n";
        // for (auto i : node) {
        //     errs() << i << " ";
        // }

        // errs() << "checking preorder num\n";
        // for (auto& i : number) {
        //     errs() << i.first << " " << i.second << "\n";
        // } errs() << "\n\n\n";



        // shrinking return vectors
        uns sumB = 0;
        for (auto iter = discovered.rbegin(); iter != discovered.rend(); ++iter) {
            if (!*iter) {
                last.pop_back(); node.pop_back(); // if not discovered, it's not been put in these
            }
        }
        last.shrink_to_fit(); node.shrink_to_fit();

        errs() << "exitted DFST\n";
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
        errs() << "LoopNesting on " << Function.getName() << "\n";
        

        auto preOrderRes = AM.getResult<ExtendedPreOrderPass>(Function);

        if (preOrderRes.last.size() == 0) {
            errs() << "mark1\n";
        }
        
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

            errs() << "uhhh FIND\n";
            return -1; // must not happen
        };

        auto UNION = [&unions](uns v, uns w) -> void {
            unions[w].merge(unions[v]);
            unions[v].clear();
        };

        auto isAncestor = [&preOrderRes](uns v, uns w) {
            return (w <= v) && (v <= preOrderRes.last[w]);
        };
        
        

        for (uns w = 0; w < preOrderRes.last.size(); ++w) {
            auto currBB = preOrderRes.node[w];
            for ( auto it = pred_begin(currBB), et = pred_end(currBB); it != et; ++it) {
                auto v = preOrderRes.number.at(*it);

                if (isAncestor(w, v)) {
                    backPreds[w].push_back(v);
                } else {
                    nonBackPreds[w].push_back(v);
                }
            } 
        }

        // header[0] = -1;

        for (int w = preOrderRes.last.size() - 1; w >= 0; --w) {
            std::set<uns> P;

            for (auto v : backPreds[w]) {
                if (v != w) {
                    P.emplace(FIND(v));
                } else {
                    type[w] = BBlock_type::self;
                }
            }

            std::vector<uns> workList(P.size());
            std::copy(P.begin(), P.end(), workList.begin());
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
                    } else if (P.find(y_) == P.end() && y_ != w) {
                        P.emplace(y_);
                        workList.push_back(y_);
                    }
                }
            }

            for (auto x : P) {
                header[x] = w;
                UNION(x, w);
            }

        }
        errs() << "exitted LoopNesting\n";

        errs() << "Headers: \n";
        for (auto a : header) errs() << a << " ";
        errs() << "\n";

        return {preOrderRes, header, type};
    }
};





// ----------------------------------------------------------------
// Main pass -- Loop Invariant Code Motion


struct LICMPass : PassInfoMixin<LICMPass> 
{
    bool isLoopMember(LoopNestingTree& LoopNesting, uns targetLoop, uns BB) {
        errs() << "entered isLoopMember \n";
        int headIter = LoopNesting.headers[BB];
        while (headIter != 0) {
            if (headIter == targetLoop) {
                errs() << "exitting with true \n";
                return true;
            }

            headIter = LoopNesting.headers[headIter];
        }
        errs() << "exitting with false \n";
        return false;
    }

    BasicBlock* insertBBBetween(std::set<BasicBlock*> predsBB, BasicBlock* succBB) {
        errs() << "entered insertBBB \n";
        auto newBB = BasicBlock::Create(succBB->getContext(), "", succBB->getParent());
        newBB->moveAfter(*predsBB.begin());

        for (auto predBB : predsBB) {
            auto terminator = predBB->getTerminator();
            terminator->replaceUsesOfWith(succBB, newBB);
        }

        IRBuilder<> Builder(newBB);
        Builder.CreateBr(succBB);
        errs() << "exitted insertBBB \n";
        return newBB;
    }

    #define insertPreheader insertBBBetween

    void moveToPreheader(LoopNestingTree& LoopNesting, Instruction* targetInst, uns targetLoop, std::set<uns>& LoopMembers) {
        errs() << "entered moveToPreheader \n";
        
        // finding entry of the loop
        auto LoopEntryPointPO = *LoopMembers.begin();
        // for (auto i : LoopMembers) {
        //     LoopEntryPointPO = std::min(LoopEntryPointPO, i);
        // }

        
        
        std::set<BasicBlock*> preheaderCandidates;
        for (auto member : LoopMembers) {
            for (auto pred : predecessors(LoopNesting.PreOrder.node[member])) {
                uns predPO = LoopNesting.PreOrder.number[pred];
                if (!isLoopMember(LoopNesting, targetLoop, predPO)) {
                    preheaderCandidates.emplace(pred);
                }
            }

            if (preheaderCandidates.size() > 0) {
                LoopEntryPointPO = member;
                break;
            }
            errs() << "switching...\n";
        }
        auto LoopEntryPoint = LoopNesting.PreOrder.node[LoopEntryPointPO];

        // if unique non-loop pred, it's preheader
        if (preheaderCandidates.size() == 1) {
            targetInst->removeFromParent();
            targetInst->insertBefore((*preheaderCandidates.begin())->getTerminator());
            errs() << "exitted, found 1 \n";
        }

        // preheader insertion needed
        else if (preheaderCandidates.size() > 1) {
            auto preheader = insertPreheader(preheaderCandidates, LoopEntryPoint);
            targetInst->removeFromParent();
            targetInst->insertBefore(preheader->getTerminator());
            errs() << "exitted, found >1 \n";
        }

        else llvm_unreachable( "zero non-loop predecessors found!\n");
    }

    bool isSafeToMove(Instruction* I) {
        if (I->mayReadOrWriteMemory())
            return false;
        if (isa<LoadInst>(I))
            return false;
        if (isa<StoreInst>(I))
            return false;
        
        // Skip terminators
        if (I->isTerminator())
            return false;
        
        // Skip PHI nodes
        if (isa<PHINode>(I))
            return false;
        
        // Skip function calls (unless proven pure)
        if (isa<CallInst>(I) || isa<InvokeInst>(I))
            return false;
        
        // Skip exception handling instructions
        if (isa<LandingPadInst>(I) || isa<CatchPadInst>(I) || isa<CleanupPadInst>(I))
            return false;

        return true;
    }


    PreservedAnalyses 
    run(Function &Function, 
	    FunctionAnalysisManager &AM) 
    {
        errs() << "\n\nfunction: " << Function.getName() << "\n";
        bool UpdateFlag = false;
        
        // when moving stuff, better keep things updated, might not be optimal
        Update:

        LoopNestingTree LoopNesting = AM.getResult<LoopNestingTreePass>(Function);

        std::map<uns, std::set<uns>> LoopMembers;
        for (uns i = 0; i < LoopNesting.headers.size(); i++) {
            if (LoopNesting.types[i] != BBlock_type::nonheader) LoopMembers[i].emplace(i);
            auto currHead = LoopNesting.headers[i];
            do {
                LoopMembers[currHead].emplace(i);
                currHead = LoopNesting.headers[currHead];
            } while (currHead != 0);
        }

        errs() <<"types: \n";
        for (uns i = 0; i < LoopNesting.types.size(); i++) {

            errs() << LoopNesting.types[i] << " " << *LoopNesting.PreOrder.node[i]->getTerminator() <<'\n';
        }
        errs() << "\n";
        // errs() << LoopNesting.headers.size() << " " << LoopNesting.types.size() << "\n";

        // iterate in reverse pre-order
        for (int BBlockPOrder = LoopNesting.headers.size() - 1; BBlockPOrder > 0; --BBlockPOrder) {
            // errs() << "lol" << BBlockPOrder << " e\n";
            // headers that suit algorithm
            if (LoopNesting.types[BBlockPOrder] == BBlock_type::reducible || 
                LoopNesting.types[BBlockPOrder] == BBlock_type::self) {

                    bool Changed = true;
                    
                    while (Changed) {
                        Changed = false;
                        
                        errs() << "here we go again\n";
                        
                        for (auto DescendantPO : LoopMembers[BBlockPOrder]) {
                            if (UpdateFlag) break;

                            if (isLoopMember(LoopNesting, BBlockPOrder, DescendantPO)) {
                                for (auto& Def : *(LoopNesting.PreOrder.node[DescendantPO])) {
                                    if (!isSafeToMove(&Def)) {
                                        continue;
                                    }

                                    bool Move = true;
                                    for (uns i = 0; i < Def.getNumOperands(); ++i) {
                                        errs() << "instruction " << Def << "\n";
                                        auto operand = Def.getOperand(i);

                                        errs() << "operand " << *operand << " at " << *LoopNesting.PreOrder.node[DescendantPO]->getTerminator() << "\n";
                                        if (llvm::isa<llvm::Constant>(operand)) {
                                            continue;
                                        }
                                        

                                        auto operandBB = dyn_cast<Instruction>(operand)->getParent();
                                        uns operandBBPO = LoopNesting.PreOrder.number[operandBB];

                                        if (isLoopMember(LoopNesting, BBlockPOrder, operandBBPO)) {
                                            Move = false; break;
                                        }
                                    }

                                    if (!Move) continue;
                                    moveToPreheader(LoopNesting, &Def, BBlockPOrder, LoopMembers[BBlockPOrder]);
                                    Changed = true;
                                    goto Update;
                                }
                            }
                        }
                    }
                }
            

        }

        errs() << "\n\n\n" << Function << "\n";


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
