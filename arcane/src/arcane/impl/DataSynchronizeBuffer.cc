﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2023 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* DataSynchronizeBuffer.cc                                    (C) 2000-2023 */
/*                                                                           */
/* Implémentation d'un buffer générique pour la synchronisation de donnéess. */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/impl/internal/DataSynchronizeBuffer.h"

#include "arcane/utils/FatalErrorException.h"
#include "arcane/impl/IBufferCopier.h"
#include "arcane/impl/DataSynchronizeInfo.h"

#include "arcane/accelerator/core/Runner.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arcane
{

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void IDataSynchronizeBuffer::
copyAllSend()
{
  Int32 nb_rank = nbRank();
  for (Int32 i = 0; i < nb_rank; ++i)
    copySendAsync(i);
  barrier();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void IDataSynchronizeBuffer::
copyAllReceive()
{
  Int32 nb_rank = nbRank();
  for (Int32 i = 0; i < nb_rank; ++i)
    copyReceiveAsync(i);
  barrier();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void DirectBufferCopier::
barrier()
{
  if (m_queue)
    m_queue->barrier();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

Int64 DataSynchronizeBufferBase::BufferInfo::
displacement(Int32 index) const
{
  return m_buffer_info->bufferDisplacement(index) * m_datatype_size;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

MutableMemoryView DataSynchronizeBufferBase::BufferInfo::
localBuffer(Int32 index)
{
  Int64 displacement = m_buffer_info->bufferDisplacement(index);
  Int32 local_size = m_buffer_info->nbItem(index);
  return m_memory_view.subView(displacement, local_size);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ConstArrayView<Int32> DataSynchronizeBufferBase::BufferInfo::
localIds(Int32 index) const
{
  return m_buffer_info->localIds(index);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

Int32 DataSynchronizeBufferBase::
targetRank(Int32 index) const
{
  return m_sync_info->targetRank(index);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void DataSynchronizeBufferBase::
barrier()
{
  m_buffer_copier->barrier();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Calcul et alloue les tampons nécessaire aux envois et réceptions
 * pour les synchronisations des variables 1D.
 */
void DataSynchronizeBufferBase::
compute(IBufferCopier* copier, DataSynchronizeInfo* sync_info, Int32 datatype_size)
{
  m_is_compare_sync_values = false;
  m_buffer_copier = copier;
  m_sync_info = sync_info;
  m_nb_rank = sync_info->size();

  m_ghost_buffer_info.m_datatype_size = datatype_size;
  m_ghost_buffer_info.m_buffer_info = &sync_info->receiveInfo();
  m_share_buffer_info.m_datatype_size = datatype_size;
  m_share_buffer_info.m_buffer_info = &sync_info->sendInfo();
  m_compare_sync_buffer_info.m_datatype_size = datatype_size;
  m_compare_sync_buffer_info.m_buffer_info = &sync_info->receiveInfo();

  IMemoryAllocator* allocator = m_buffer_copier->allocator();
  if (allocator && allocator != m_buffer.allocator())
    m_buffer = UniqueArray<std::byte>(allocator);

  _allocateBuffers(datatype_size);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Calcul et alloue les tampons nécessaires aux envois et réceptions
 * pour les synchronisations des variables 1D.
 *
 * \todo: ne pas converver les tampons pour chaque type de donnée des variables
 * car leur conservation est couteuse en terme de memoire.
 */
void DataSynchronizeBufferBase::
_allocateBuffers(Int32 datatype_size)
{
  Int64 total_ghost_buffer = m_sync_info->receiveInfo().totalNbItem();
  Int64 total_share_buffer = m_sync_info->sendInfo().totalNbItem();

  Int32 full_dim2_size = datatype_size;
  Int64 total_size = total_ghost_buffer + total_share_buffer;
  if (m_is_compare_sync_values)
    total_size += total_ghost_buffer;
  m_buffer.resize(total_size * full_dim2_size);

  Int64 share_offset = total_ghost_buffer * full_dim2_size;
  Int64 check_sync_offset = share_offset + total_share_buffer * full_dim2_size;

  auto s1 = m_buffer.span().subspan(0, share_offset);
  m_ghost_buffer_info.m_memory_view = makeMutableMemoryView(s1.data(), full_dim2_size, total_ghost_buffer);
  auto s2 = m_buffer.span().subspan(share_offset, total_share_buffer * full_dim2_size);
  m_share_buffer_info.m_memory_view = makeMutableMemoryView(s2.data(), full_dim2_size, total_share_buffer);
  if (m_is_compare_sync_values) {
    auto s3 = m_buffer.span().subspan(check_sync_offset, total_ghost_buffer * full_dim2_size);
    m_compare_sync_buffer_info.m_memory_view = makeMutableMemoryView(s3.data(), full_dim2_size, total_ghost_buffer);
  }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void SingleDataSynchronizeBuffer::
copyReceiveAsync(Int32 index)
{
  m_ghost_buffer_info.checkValid();
  ARCANE_CHECK_POINTER(m_buffer_copier);

  MutableMemoryView var_values = dataView();
  ConstArrayView<Int32> indexes = m_ghost_buffer_info.localIds(index);
  ConstMemoryView local_buffer = m_ghost_buffer_info.localBuffer(index);

  m_buffer_copier->copyFromBufferAsync(indexes, local_buffer, var_values);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void SingleDataSynchronizeBuffer::
copySendAsync(Int32 index)
{
  ARCANE_CHECK_POINTER(m_buffer_copier);
  m_share_buffer_info.checkValid();

  ConstMemoryView var_values = dataView();
  ConstArrayView<Int32> indexes = m_share_buffer_info.localIds(index);
  MutableMemoryView local_buffer = m_share_buffer_info.localBuffer(index);
  m_buffer_copier->copyToBufferAsync(indexes, local_buffer, var_values);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void SingleDataSynchronizeBuffer::
prepareSynchronize(bool is_compare_sync)
{
  m_is_compare_sync_values = is_compare_sync;
  if (!is_compare_sync)
    return;
  // Recopie dans le buffer de vérification les valeurs actuelles des mailles
  // fantômes.
  MutableMemoryView var_values = dataView();
  Int32 nb_rank = nbRank();
  for (Int32 i = 0; i < nb_rank; ++i) {
    ConstArrayView<Int32> indexes = m_compare_sync_buffer_info.localIds(i);
    MutableMemoryView local_buffer = m_compare_sync_buffer_info.localBuffer(i);
    m_buffer_copier->copyToBufferAsync(indexes, local_buffer, var_values);
  }
  // Normalement pas besoin de faire une barrière car ensuite il y aura les
  // envois sur la même \a queue et ensuite une barrière.
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Compare les valeurs avant/après synchronisation.
 *
 * Il suffit de comparer bit à bit le buffer de réception avec celui
 * contenant les valeurs avant la synchronisation (m_check_sync_buffer).
 *
 * \retval \a vrai s'il y a des différences.
 */
bool SingleDataSynchronizeBuffer::
compareCheckSynchronize()
{
  if (!m_is_compare_sync_values)
    return false;
  ConstMemoryView reference_buffer = m_compare_sync_buffer_info.globalBuffer();
  ConstMemoryView receive_buffer = m_ghost_buffer_info.globalBuffer();
  Span<const std::byte> reference_bytes = reference_buffer.bytes();
  Span<const std::byte> receive_bytes = receive_buffer.bytes();
  Int64 reference_size = reference_bytes.size();
  Int64 receive_size = receive_bytes.size();
  if (reference_size != receive_size)
    ARCANE_FATAL("Incoherent buffer size ref={0} receive={1}", reference_size, receive_size);
  // TODO: gérer le cas où la mémoire est sur le device
  return std::memcmp(reference_bytes.data(), receive_bytes.data(), reference_size) != 0;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void MultiDataSynchronizeBuffer::
copyReceiveAsync(Int32 index)
{
  ARCANE_CHECK_POINTER(m_buffer_copier);
  m_ghost_buffer_info.checkValid();

  Int64 data_offset = 0;
  Span<const std::byte> local_buffer_bytes = m_ghost_buffer_info.localBuffer(index).bytes();
  Int32ConstArrayView indexes = m_ghost_buffer_info.localIds(index);
  const Int64 nb_element = indexes.size();
  for (MutableMemoryView var_values : m_data_views) {
    Int32 datatype_size = var_values.datatypeSize();
    Int64 current_size_in_bytes = nb_element * datatype_size;
    Span<const std::byte> sub_local_buffer_bytes = local_buffer_bytes.subSpan(data_offset, current_size_in_bytes);
    ConstMemoryView local_buffer = makeConstMemoryView(sub_local_buffer_bytes.data(), datatype_size, nb_element);
    if (current_size_in_bytes != 0)
      m_buffer_copier->copyFromBufferAsync(indexes, local_buffer, var_values);
    data_offset += current_size_in_bytes;
  }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void MultiDataSynchronizeBuffer::
copySendAsync(Int32 index)
{
  ARCANE_CHECK_POINTER(m_buffer_copier);
  m_ghost_buffer_info.checkValid();

  Int64 data_offset = 0;
  Span<std::byte> local_buffer_bytes = m_share_buffer_info.localBuffer(index).bytes();
  Int32ConstArrayView indexes = m_share_buffer_info.localIds(index);
  const Int64 nb_element = indexes.size();
  for (ConstMemoryView var_values : m_data_views) {
    Int32 datatype_size = var_values.datatypeSize();
    Int64 current_size_in_bytes = nb_element * datatype_size;
    Span<std::byte> sub_local_buffer_bytes = local_buffer_bytes.subSpan(data_offset, current_size_in_bytes);
    MutableMemoryView local_buffer = makeMutableMemoryView(sub_local_buffer_bytes.data(), datatype_size, nb_element);
    if (current_size_in_bytes != 0)
      m_buffer_copier->copyToBufferAsync(indexes, local_buffer, var_values);
    data_offset += current_size_in_bytes;
  }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // namespace Arcane

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
