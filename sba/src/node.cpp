#include <sba/node.h>

using namespace std;
using namespace Eigen;

namespace sba
{
    void Node::setTransform()
    { transformW2F(w2n,trans,qrot); }

  //
  // sets incremental angle derivatives
  //
  void Node::setDri()
  {
    setDr(true);
  }

  // What is this line for????
  // these are the derivatives of the *inverse* rotation
  Eigen::Matrix3d Node::dRidx, Node::dRidy, Node::dRidz;

  void Node::initDr()
  {
    dRidx  << 0.0,0.0,0.0,  
              0.0,0.0,2.0,
              0.0,-2.0,0.0;
    dRidy  << 0.0,0.0,-2.0,
              0.0,0.0,0.0,
              2.0,0.0,0.0;
    dRidz  << 0.0,2.0,0.0,  
              -2.0,0.0,0.0,
              0.0,0.0,0.0;
  }


  //
  // sets angle derivatives
  //
  void Node::setDr(bool local)
  {
    // for dS'*R', with dS the incremental change
    if (local)
      {
#if 0
        dRdx = w2n.block<3,3>(0,0) * dRidx;
        dRdy = w2n.block<3,3>(0,0) * dRidy;
        dRdz = w2n.block<3,3>(0,0) * dRidz;
#endif
        dRdx = dRidx * w2n.block<3,3>(0,0);
        dRdy = dRidy * w2n.block<3,3>(0,0);
        dRdz = dRidz * w2n.block<3,3>(0,0);

      }
    else
      {
        double x2 = qrot.x() * 2.0;
        double y2 = qrot.y() * 2.0;
        double z2 = qrot.z() * 2.0;
        double w2 = qrot.w() * 2.0;
        double xw = qrot.x()/qrot.w(); // these are problematic for w near zero
        double yw = qrot.y()/qrot.w();
        double zw = qrot.z()/qrot.w();

        // dR/dx 
        dRdx(0,0) = 0.0;
        dRdx(0,1) = y2-zw*x2;
        dRdx(0,2) = z2+yw*x2;

        dRdx(1,0) = y2+zw*x2;
        dRdx(1,1) = -2.0*x2;
        dRdx(1,2) = w2-xw*x2;

        dRdx(2,0) = z2-yw*x2;
        dRdx(2,1) = -dRdx(1,2);
        dRdx(2,2) = dRdx(1,1);
      
        // dR/dy 
        dRdy(0,0) = -2.0*y2;
        dRdy(0,1) = x2-zw*y2;
        dRdy(0,2) = (-w2)+yw*y2;

        dRdy(1,0) = x2+zw*y2;
        dRdy(1,1) = 0.0;
        dRdy(1,2) = dRdx(2,0);

        dRdy(2,0) = -dRdy(0,2);
        dRdy(2,1) = dRdx(0,2);
        dRdy(2,2) = dRdy(0,0);

        // dR/dz
        dRdz(0,0) = -2.0*z2;
        dRdz(0,1) = w2-zw*z2;
        dRdz(0,2) = dRdy(1,0);

        dRdz(1,0) = -dRdz(0,1);
        dRdz(1,1) = dRdz(0,0);
        dRdz(1,2) = dRdx(0,1);

        dRdz(2,0) = dRdy(0,1);
        dRdz(2,1) = dRdx(1,0);
        dRdz(2,2) = 0.0;
      }
  }
  
  void Node::normRotLocal()
  {
      qrot.normalize();
      if (qrot.w() < 0) qrot.coeffs().start<3>() = -qrot.coeffs().start<3>();
      if (isnan(qrot.x()) || isnan(qrot.y()) || isnan(qrot.z()) || isnan(qrot.w()) )
        { printf("[NormRot] Bad quaternion\n"); *(int *)0x0 = 0; }
      //      std::cout << "[NormRot] qrot end   = " << qrot.transpose() << std::endl;
   }

  // transforms
  void transformW2F(Eigen::Matrix<double,3,4> &m, 
                    const Eigen::Matrix<double,4,1> &trans, 
                    const Eigen::Quaternion<double> &qrot)
  {
    m.block<3,3>(0,0) = qrot.toRotationMatrix().transpose();
    m.col(3).setZero();         // make sure there's no translation
    m.col(3) = -m*trans;
  };

  void transformF2W(Eigen::Matrix<double,3,4> &m, 
                    const Eigen::Matrix<double,4,1> &trans, 
                    const Eigen::Quaternion<double> &qrot)
  {
    m.block<3,3>(0,0) = qrot.toRotationMatrix();
    m.col(3) = trans.start(3);
  };


  // returns the local R,t in nd0 that produces nd1
  // NOTE: returns a postfix rotation; should return a prefix
  void transformN2N(Eigen::Matrix<double,4,1> &trans, 
                    Eigen::Quaternion<double> &qr,
                    Node &nd0, Node &nd1)
  {
    Matrix<double,3,4> tfm;
    Quaterniond q0,q1;
    q0 = nd0.qrot;
    transformW2F(tfm,nd0.trans,q0);
    trans.start(3) = tfm*nd1.trans;
    trans(3) = 1.0;
    q1 = nd1.qrot;
    qr = q0.inverse()*q1;
    qr.normalize();
    if (qr.w() < 0)
      qr.coeffs() = -qr.coeffs();
  }


} // namespace sba

