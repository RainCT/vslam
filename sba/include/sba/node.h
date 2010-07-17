#ifndef _NODE_H_
#define _NODE_H_

// Functions for handling nodes (cameras) within the sba system.

#include <stdio.h>
#include <iostream>
#include <Eigen/Core>
#include <Eigen/Geometry>

// needed for camera params
#ifndef _CAMPARAMS_H_
#define _CAMPARAMS_H_

namespace frame_common
{

  /// camera params
  /// focal lengths, image center, baseline (for stereo)
  /// duplicated in SBA package
  struct CamParams
  {
    double fx, fy, cx, cy, tx;
  };
} // end namespace

#endif // _CAMPARMS_H_

namespace fc = frame_common;

// put things into a namespace
namespace sba
{

  /// Keypoints - subpixel using floats.
  ///
  /// u,v are pixel coordinates, d is disparity (if stereo)
  typedef Eigen::Vector3d Keypoint;


  /// POINT holds 3D points using in world coordinates.
  /// Currently we just represent these as 4-vectors, with a final
  /// "1.0".
  typedef Eigen::Vector4d Point;


  /// NODE holds graph nodes corresponding to frames, for use in
  /// sparse bundle adjustment 

  /// type double must be <double> or <float>
  ///
  /// Each node has a 6DOF pose, encoded as a translation vector and
  /// rotation unit quaternion (Eigen classes).  These represent the
  /// pose of the node in the world frame.
  /// 
  /// The pose generates a 3x4 homogenous transform, taking a point
  /// in world coordinates into the node coordinates.
  /// 
  /// Additionally a 3x4 homogenous transform is composed from the
  /// pose transform and a projection transform to the frame image
  /// coordinates. 
  ///
  /// Projections from points to features are in a list.  There
  /// should also be a reverse index from features to projections
  /// and points, so that matched features can tie in to points.
  ///
  class Node
  {
  public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW // needed for 16B alignment

    /// 6DOF pose as a unit quaternion and translation vector
    Eigen::Matrix<double,4,1> trans;    // homogeneous coordinates, last element is 1.0
    Eigen::Quaternion<double> qrot;     // this is a unit quaternion as coefficients, note xyzw order
    
    /// Normalize quaternion coefficient to unit;
    /// problem with derivatives near w=0, solved by a hack for now
    inline 
      void normRot()
    { 
      //      std::cout << "[NormRot] qrot start = " << qrot.transpose() << std::endl;
      if (qrot.w() < 0) qrot.coeffs().start<3>() = -qrot.coeffs().start<3>();
      double sn = qrot.coeffs().start<3>().squaredNorm();
      if (sn >= 0.9999)            // too close to high derivatives
        qrot.coeffs().start<3>() *= -1.0/(sqrt(sn)*1.0001); // switch sides; 1e-4 seems to work well
      qrot.w() = sqrt(1.0 - qrot.coeffs().start<3>().squaredNorm());
      if (isnan(qrot.x()) || isnan(qrot.y()) || isnan(qrot.z()) || isnan(qrot.w()) )
        { printf("[NormRot] Bad quaternion\n"); *(int *)0x0 = 0; }
      //      std::cout << "[NormRot] qrot end   = " << qrot.transpose() << std::endl;
    }
    
    void normRotLocal();

    /// Resultant transform from world to node coordinates;
    Eigen::Matrix<double,3,4> w2n;
    void setTransform();

    /// Covariance matrix, 6x6.  Variables are [trans,rot], with the
    /// rotational part being the xyz parameters of the unit
    /// quaternion
    //    Eigen::Matrix<double,6,6> covar;

    /// Projection to frame image coordinates;
    /// pre-multiply by the frame camera matrix
    Eigen::Matrix<double,3,3> Kcam; 
    double baseline;            // baseline for stereo
    void setKcam(const fc::CamParams &cp)
    {
      Kcam.setZero();
      Kcam(0,0) = cp.fx;
      Kcam(1,1) = cp.fy;
      Kcam(0,2) = cp.cx;
      Kcam(1,2) = cp.cy;
      Kcam(2,2) = 1.0;
      baseline = cp.tx;
      setProjection();
    }

    /// projects <p> onto <pi>
    Eigen::Matrix<double,3,4> w2i;
    void project2im(Eigen::Vector2d &pi, const Point &p)
          { Eigen::Vector3d p1 = w2i * p; pi = p1.start(2)/p1(2); }

    /// just set up projection matrix, assumes frame is present
    void setProjection()
          { w2i = Kcam * w2n; }

    /// Derivatives of the rotation matrix transpose wrt quaternion xyz, used for
    /// calculating Jacobian wrt pose of a projection.
    Eigen::Matrix<double,3,3> dRdx, dRdy, dRdz;
    static Eigen::Matrix3d dRidx, dRidy, dRidz; // these are constant
    static void initDr();       // call this to set up constant matrices

    void setDr(bool local = false); // set angle derivatives
    void setDri();              // set local angle derivatives

    /// For SBA, is this camera fixed or free?
    bool isFixed;

    /// 6DOF pose as a unit quaternion and translation vector, saving
    /// for LM step
    Eigen::Matrix<double,4,1> oldtrans; // homogeneous coordinates, last element is 1.0
    Eigen::Quaternion<double> oldqrot;  // this is the quaternion as coefficients, note xyzw order
  };
  
  
  // Functions to create transformations

  /// Projection matrix from World to Frame coordinates: [R' -R'*t]
  ///
  /// Based on translation vector and unit quaternion rotation, which
  /// gives the frame's pose in world coordinates.  Assumes quaternion
  /// is normalized (unit).  
  ///

  void transformW2F(Eigen::Matrix<double,3,4> &m, 
                    const Eigen::Matrix<double,4,1> &trans, 
                    const Eigen::Quaternion<double> &qrot);


  /// Projection matrix from Frame to World coordinates: [R t]
  ///
  void transformF2W(Eigen::Matrix<double,3,4> &m, 
                    const Eigen::Matrix<double,4,1> &trans, 
                    const Eigen::Quaternion<double> &qrot);


  /// Transform of one node in another's coords
  ///
  void transformN2N(Eigen::Matrix<double,4,1> &trans, 
                    Eigen::Quaternion<double> &qrot,
                    Node &nd0, Node &nd1);

} // sba namespace

#endif // _NODE_H_