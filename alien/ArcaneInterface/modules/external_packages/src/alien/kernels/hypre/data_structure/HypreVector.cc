#include "HypreVector.h"
/* Author : havep at Wed Jul 18 14:08:21 2012
 * Generated by createNew
 */

#include <alien/kernels/hypre/HypreBackEnd.h>
#include <alien/kernels/hypre/data_structure/HypreInternal.h>

#include <arccore/message_passing_mpi/MpiMessagePassingMng.h>

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Alien {

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

HypreVector::HypreVector(const MultiVectorImpl* multi_impl)
: IVectorImpl(multi_impl, AlgebraTraits<BackEnd::tag::hypre>::name())
, m_internal(nullptr)
, m_block_size(1)
, m_offset(0)
{
  ;
}

/*---------------------------------------------------------------------------*/

HypreVector::~HypreVector()
{
  delete m_internal;
}

/*---------------------------------------------------------------------------*/

void
HypreVector::init(const VectorDistribution& dist, const bool need_allocate)
{
  const Block* block = this->block();
  if (this->block())
    m_block_size *= block->size();
  else if (this->vblock())
    throw Arccore::FatalErrorException(A_FUNCINFO, "Not implemented yet");
  else
    m_block_size = 1;
  m_offset = dist.offset();
  if (need_allocate)
    allocate();
}

/*---------------------------------------------------------------------------*/

void
HypreVector::allocate()
{
  delete m_internal;
  const VectorDistribution& dist = this->distribution();
  auto* pm = dynamic_cast<Arccore::MessagePassing::Mpi::MpiMessagePassingMng*>(
      dist.parallelMng());
  if (*static_cast<const MPI_Comm*>(pm->getMPIComm()) != MPI_COMM_NULL)
    m_internal = new VectorInternal(*static_cast<const MPI_Comm*>(pm->getMPIComm()));
  else
    m_internal = new VectorInternal(MPI_COMM_WORLD);
  int ilower = dist.offset() * m_block_size;
  int iupper = ilower + dist.localSize() * m_block_size - 1;
  m_internal->init(ilower, iupper);
  m_rows.resize(dist.localSize() * m_block_size);
  for (int i = 0; i < dist.localSize() * m_block_size; ++i)
    m_rows[i] = ilower + i;
}

/*---------------------------------------------------------------------------*/

bool
HypreVector::setValues(const int nrow, const int* rows, const double* values)
{
  if (m_internal == NULL)
    return false;
  return m_internal->setValues(nrow, rows, values);
}

/*---------------------------------------------------------------------------*/

bool
HypreVector::setValues(const int nrow, const double* values)
{
  if (m_internal == NULL)
    return false;

  return m_internal->setValues(m_rows.size(), m_rows.data(), values);
}

/*---------------------------------------------------------------------------*/

bool
HypreVector::getValues(const int nrow, const int* rows, double* values) const
{
  if (m_internal == NULL)
    return false;
  return m_internal->getValues(nrow, rows, values);
}

/*---------------------------------------------------------------------------*/

bool
HypreVector::getValues(const int nrow, double* values) const
{
  if (m_internal == NULL)
    return false;
  return m_internal->getValues(m_rows.size(), m_rows.data(), values);
}

/*---------------------------------------------------------------------------*/

bool
HypreVector::assemble()
{
  if (m_internal == NULL)
    return false;
  return m_internal->assemble();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void
HypreVector::update(const HypreVector& v)
{
  ALIEN_ASSERT((this == &v), ("Unexpected error"));
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // namespace Alien

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/