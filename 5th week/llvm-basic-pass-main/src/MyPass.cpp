/**
 * Скелет прохода для LLVM.
 */
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include <map>
#include <string>
#include <stack>
#include <vector>

#define uns unsigned

using namespace llvm;

/* Так как мы пишем библиотеку, то можно выбрать анонимную область видимости */
namespace {
 
/**
 * Определяет, что будем делать при посещении функции. 
 */
void 
VisitFunction( Function &F) 
{
    /* LLVM использует кастомные потоки вывода. 
     * llvm::errs() (поток ошибок), 
     * llvm::outs() (аналогичен стандартному stdout), 
     * llvm::nulls() (отбрасывает весь вывод) */
    errs() << "Function name: " << F.getName() << "\n";
    errs() << "    number of arguments: " << F.arg_size() << "\n";
} /* VisitFunction */

/**
 * Структура, необходимая для регистрации своего прохода в менеджере проходов.
 */
// struct MyPass : PassInfoMixin<MyPass> 
// {
//     /* PreservedAnalyses - множество анализов, которые сохраняются после данного прохода,
//      * чтобы не запускать их заново.
//      *
//      * run() непосредственно нужен для запуска прохода.
//      *
//      * Так как мы просто хотим вывести: "имя функции - количество аргументов", то мы 
//      * возвращаем all(), что говорит о том, что ни один анализ не будет испорчен */
//     PreservedAnalyses 
//     run( Function &Function, 
// 	 FunctionAnalysisManager &AnalysisManager) 
//     {
//         VisitFunction( Function);
//         return (PreservedAnalyses::all());
//     }

//     /* По умолчанию данный проход будет пропущен, если функция помечена атрибутом 
//      * optnone (не производить оптимизаций над ней). Поэтому необходимо вернуть true, 
//      * чтобы мы могли обходить и их. 
//      * (в режиме сборки -O0 все функции помечены как неоптимизируемые) */
//     static bool 
//     isRequired( void) 
//     { 
//         return (true); 
//     }
// };

void numOfOperatorUses(Function& func) {
    std::map<std::string, uns> numOfUses;

    for (auto& BB: func) {
        for (auto& Inst: BB) {
            std::string Opcode = Inst.getOpcodeName();
            
            if (numOfUses.find(Opcode) != numOfUses.end()) 
                numOfUses.at(Opcode) += 1;
            
            else {
                numOfUses.emplace(Opcode, 1);
            }

        }
    }

    for (auto& out : numOfUses) {
        errs() << out.first << ": " << out.second << "\n";
    }
    errs() << "\n";
}

struct MyPass : PassInfoMixin<MyPass> 
{
    PreservedAnalyses 
    run( Function &Function, 
	 FunctionAnalysisManager &AnalysisManager) 
    {
        errs() << "-------------------------------\n";
        errs() << "function: " << Function.getName() << "\n";

        numOfOperatorUses(Function);

        for (auto& BB: Function) {
            errs() << BB.getNumber() << " with " << *(BB.getTerminator()) << "\n";
        }
        errs() << "\n";

        auto& entry = Function.getEntryBlock();
        std::vector<int> discovered(Function.size(), false); 

        std::stack<llvm::BasicBlock*> workStack;
        std::stack<uns> RPO;

        workStack.push(&entry);
        while (!workStack.empty()) {
            auto curr = workStack.top();

            if (!discovered[curr->getNumber()]) {
                discovered[curr->getNumber()] = true;

                auto currT = curr->getTerminator();
                for (uns i = 0, n = currT->getNumSuccessors(); i < n; i++)
                    if (!discovered[currT->getSuccessor(i)->getNumber()])
                        workStack.push(currT->getSuccessor(i));
                
                continue;
            }

            workStack.pop();
            RPO.push(curr->getNumber());
        }
        
        // unpacking
        while (!RPO.empty()) {
            errs() << RPO.top() << " ";
            RPO.pop();
        }
        errs() << "\n";
        errs() << "-------------------------------\n";
        return (PreservedAnalyses::all());
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
    if ( Name == "MyPass" )
    {
        FPM.addPass( MyPass());
	return (true);
    } else
    {
        return (false);
    }
} /* CallBackForPipelineParser */

void
CallBackForPassBuilder( PassBuilder &PB)
{
    PB.registerPipelineParsingCallback( &CallBackForPipelineParser); 
} /* CallBackForPassBuider */

PassPluginLibraryInfo 
getMyPassPluginInfo( void)
{
    uint32_t     APIversion =  LLVM_PLUGIN_API_VERSION;
    const char * PluginName =  "MyPass";
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
  return (getMyPassPluginInfo());
} /* llvmGetPassPluginInfo */
