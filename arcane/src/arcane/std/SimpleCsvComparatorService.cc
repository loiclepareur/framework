﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2022 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* SimpleCsvComparatorService.cc                                   (C) 2000-2022 */
/*                                                                           */
/* Service permettant de construire et de sortir un tableau au formet csv.   */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/std/SimpleCsvComparatorService.h"

#include <arcane/Directory.h>
#include <arcane/IMesh.h>
#include <arcane/IParallelMng.h>

#include <optional>
#include <string>

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arcane
{

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void SimpleCsvComparatorService::
init(ISimpleTableOutput* ptr_sto)
{
  ARCANE_CHECK_PTR(ptr_sto);
  m_iSTO = ptr_sto;
  // On déduit la position des tableaux de références avec le STO qu'on 
  // nous a donné.
  m_path_ref_str = m_iSTO->dir()+"_ref";
  m_path_ref = Directory(m_iSTO->rootPathOutput(), m_path_ref_str);
  m_name_ref = m_iSTO->nameFile();
}

void SimpleCsvComparatorService::
editRefFileEntry(String path, String name)
{
  m_path_ref = Directory(path);
  m_name_ref = name;
}

bool SimpleCsvComparatorService::
writeRefFile(Integer only_proc)
{
  return m_iSTO->writeFile(m_path_ref_str, only_proc);
}

bool SimpleCsvComparatorService::
isRefExist(Integer only_proc)
{
  if (only_proc != -1 && mesh()->parallelMng()->commRank() != only_proc)
    return false;
  _openFile(m_name_ref);
  info() << "file : " << m_name_ref << " " << m_ifstream.good();
  return m_ifstream.good();
}

bool SimpleCsvComparatorService::
readRefFile(Integer only_proc)
{
  // Pas de fichier, pas de chocolats.
  if(!isRefExist(only_proc)) return false;
  info() << "Pass 1";

  std::string line;

  // S'il n'y a pas de première ligne, on arrete là.
  // Un fichier écrit par SimpleCsvOutput possède toujours au
  // moins une ligne.
  if(!std::getline(m_ifstream, line)) return false;
  info() << "Pass 2";

  // Sinon, on a la ligne d'entête, contenant les noms
  // des colonnes (et le nom du tableau).
  String ligne(line);
  ligne.split(m_name_columns_with_name_of_tab, ';');

  // S'il n'y a pas d'autres lignes, c'est qu'il n'y a que des 
  // colonnes vides (ou aucunes colonnes) et aucunes lignes.
  if(!std::getline(m_ifstream, line)) return true;
  info() << "Pass 3";

  Integer num_columns = m_name_columns_with_name_of_tab.size()-1;

  m_values_csv.resize(1, num_columns);


  Integer compt_line = 0;

  do{
    StringUniqueArray splitted_line;
    m_values_csv.resize(compt_line+1);
    String ligne(line);
    ligne.split(splitted_line, ';');
    m_name_rows.add(splitted_line[0]);
    for(Integer i = 1; i < splitted_line.size(); i++){
      m_values_csv[compt_line][i-1] = std::stold(splitted_line[i].localstr());
    }
    compt_line++;
  } while(std::getline(m_ifstream, line));
  info() << "Pass 4";

  return true;
}

// TODO DEBUG A suppr.
void SimpleCsvComparatorService::
print()
{
  String m_separator = ";";

  for (Integer j = 0; j < m_name_columns_with_name_of_tab.size(); j++) {
    std::cout << m_name_columns_with_name_of_tab[j] << m_separator;
  }
  std::cout << std::endl;

  for (Integer i = 0; i < m_values_csv.dim1Size(); i++) {
      std::cout << m_name_rows[i];
    std::cout << m_separator;
    ConstArrayView<Real> view = m_values_csv[i];
    for (Integer j = 0; j < m_values_csv.dim2Size(); j++) {
      std::cout << view[j] << m_separator;
    }
    std::cout << std::endl;
  }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void SimpleCsvComparatorService::
_openFile(String name_file)
{
  if(m_is_file_open) return;
  m_ifstream.open(m_path_ref.file(name_file).localstr(), std::ifstream::in);
  m_is_file_open = true;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Arcane

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
