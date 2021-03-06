/*=========================================================================
  Program:   ShapeWorks: Particle-based Shape Correspondence & Visualization
  Module:    $RCSfile: itkPSMProcrustesRegistration.h,v $
  Date:      $Date: 2011/03/24 01:17:33 $
  Version:   $Revision: 1.3 $
  Author:    $Author: wmartin $

  Copyright (c) 2009 Scientific Computing and Imaging Institute.
  See ShapeWorksLicense.txt for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.
=========================================================================*/
#ifndef __itkPSMProcrustesRegistration_h
#define __itkPSMProcrustesRegistration_h

#include "itkDataObject.h"
#include "itkWeakPointer.h"
#include "vnl/vnl_matrix.h"
#include "itkPSMParticleSystem.h"
#include "itkPSMProcrustesFunction.h"

namespace itk
{
/** \class PSMProcrustesRegistration
 *  \brief This class interfaces with the PSMProcrustesFunction class 
 *  to register a list of point sets. It transforms the registration transforms
 *  generated by PSMProcrustesFunction from Configuration space to 
 *  Procrustes space. The resulting transforms are then applied to the point sets.
 *  
 *
 *  \ingroup PSM
 */
template <unsigned int VDimension>
class ITK_EXPORT PSMProcrustesRegistration: public DataObject
{
public:
  /** Standard class typedefs */
  //  typedef TDataType DataType;
  typedef PSMProcrustesRegistration Self;
  typedef DataObject Superclass;
  typedef SmartPointer<Self>  Pointer;
  typedef SmartPointer<const Self>  ConstPointer;
  typedef WeakPointer<const Self>  ConstWeakPointer;

  /** Particle system typedefs. */
  typedef PSMParticleSystem<VDimension> PSMParticleSystemType;
  typedef typename PSMParticleSystemType::TransformType TransformType;
  
  /** Procrustes Function typedef */
  typedef PSMProcrustesFunction<VDimension> PSMProcrustesFunctionType;
  typedef typename PSMProcrustesFunctionType::ShapeListType ShapeListType;
  typedef typename PSMProcrustesFunctionType::ShapeType ShapeType;
  typedef typename PSMProcrustesFunctionType::PointType PointType;
  typedef typename PSMProcrustesFunctionType::SimilarityTransformListType SimilarityTransformListType;
  /** Method for creation through the object factory. */
  itkNewMacro(Self);
  
  /** Run-time type information (and related methods). */
  itkTypeMacro(PSMProcrustesRegistration, DataObject);

  /** Set/Get the target particle system. */
  void SetPSMParticleSystem(PSMParticleSystemType *p)
  { m_PSMParticleSystem = p;  }
  const PSMParticleSystemType *GetPSMParticleSystem() const
  { return m_PSMParticleSystem; }
  PSMParticleSystemType *GetPSMParticleSystem()
  { return m_PSMParticleSystem; }

  /** Callback suitable for adding as an observer of itk object iteration
      events. */
  void IterationCallback(itk::Object *, const itk::EventObject &)
  {
    this->RunRegistration();
  }

  /** Performs a procrustes registration and modifies the transforms of the
      particle system accordingly.  Assumes m_ParticleSystem has been set to
      point to a valid object.*/
  void RunRegistration(int i);
  void RunRegistration()
  {
    for (int i = 0; i < m_DomainsPerShape; i++)
      {
      this->RunRegistration(i);
      }
  }

  /** Set/Get the number of Domains in each shape.  Procrustes will be
      performed separately for each domain. */
  void SetDomainsPerShape(int i)
  { m_DomainsPerShape = i; }
  int GetDomainsPerShape() const
  { return m_DomainsPerShape; }

  /** Turn on/off the scaling. Shapes will only be rotated and translated if 
   scaling is turned off.*/
  bool GetScaling() const
  { return m_Scaling; }
  void ScalingOn()
  { m_Scaling = true; }
  void ScalingOff()
  { m_Scaling = false; }

  /** Set/Get the number (interval) of iterations at which GPA is run. */
  void SetProcrustesInterval(int i)
  { m_Procrustes_Interval = i; }
  int GetProcrustesInterval()
  { return m_Procrustes_Interval; }


protected:
  PSMProcrustesRegistration() : m_Procrustes_Interval(1),
     m_DomainsPerShape(1), m_Scaling(true)
    {  }
  virtual ~PSMProcrustesRegistration() {};

  void PrintSelf(std::ostream& os, Indent indent) const
  {  Superclass::PrintSelf(os,indent);  }

private:
  PSMProcrustesRegistration(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  /** The number (interval) of iterations when GPA is run. */
  int m_Procrustes_Interval;

  /** The domains per shape. */
  int m_DomainsPerShape;

  /** Variable to turn on/off the scaling. */
  bool m_Scaling;
  
  /** Particle System Pointer that points to the list of point correspondences. */
  PSMParticleSystemType *m_PSMParticleSystem;
};

} // end namespace

#endif
