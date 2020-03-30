/* Author : desrozis at Mon Mar 30 15:06:37 2009
 * Generated by createNew
 */

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include <alien/kernels/ifp/linear_solver/arcane/IFPLinearSolverService.h>

#include <ALIEN/axl/IFPLinearSolver_StrongOptions.h>

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
namespace Alien {

#ifdef ALIEN_USE_ARCANE
IFPLinearSolverService::IFPLinearSolverService(const Arcane::ServiceBuildInfo& sbi)
: ArcaneIFPLinearSolverObject(sbi)
, Alien::IFPInternalLinearSolver(
      sbi.subDomain()->parallelMng()->messagePassingMng(), options())
{
}
#endif

IFPLinearSolverService::IFPLinearSolverService(
    Arccore::MessagePassing::IMessagePassingMng* parallel_mng,
    std::shared_ptr<IOptionsIFPLinearSolver> _options)
: ArcaneIFPLinearSolverObject(_options)
, Alien::IFPInternalLinearSolver(parallel_mng, options())
{
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ARCANE_REGISTER_SERVICE_IFPLINEARSOLVER(IFPSolver, IFPLinearSolverService);

} // namespace Alien

REGISTER_STRONG_OPTIONS_IFPLINEARSOLVER();
