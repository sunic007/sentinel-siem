import React from 'react';
import { Routes, Route, Navigate, useNavigate, useLocation } from 'react-router-dom';
import { Layout, Menu, Typography } from 'antd';
import {
  DashboardOutlined,
  SearchOutlined,
  AlertOutlined,
  FileProtectOutlined,
  RadarChartOutlined,
  DesktopOutlined,
  SettingOutlined,
  BugOutlined,
} from '@ant-design/icons';

import SecurityPosture from './pages/SecurityPosture';
import IncidentReview from './pages/IncidentReview';
import ThreatIntelligence from './pages/ThreatIntelligence';
import AssetCenter from './pages/AssetCenter';
import RiskAnalysis from './pages/RiskAnalysis';
import Search from './pages/Search';
import Settings from './pages/Settings';

const { Sider, Content, Header } = Layout;
const { Title } = Typography;

const menuItems = [
  { key: '/posture', icon: <DashboardOutlined />, label: 'Security Posture' },
  { key: '/search', icon: <SearchOutlined />, label: 'Search' },
  { key: '/incidents', icon: <AlertOutlined />, label: 'Incident Review' },
  { key: '/threat-intel', icon: <BugOutlined />, label: 'Threat Intelligence' },
  { key: '/assets', icon: <DesktopOutlined />, label: 'Asset Center' },
  { key: '/risk', icon: <RadarChartOutlined />, label: 'Risk Analysis' },
  { key: '/settings', icon: <SettingOutlined />, label: 'Settings' },
];

const App: React.FC = () => {
  const navigate = useNavigate();
  const location = useLocation();

  return (
    <Layout style={{ minHeight: '100vh' }}>
      <Sider
        width={220}
        style={{
          background: '#141414',
          borderRight: '1px solid #303030',
        }}
      >
        <div
          style={{
            padding: '16px 20px',
            borderBottom: '1px solid #303030',
            display: 'flex',
            alignItems: 'center',
            gap: 10,
          }}
        >
          <FileProtectOutlined style={{ fontSize: 24, color: '#1668dc' }} />
          <Title level={4} style={{ margin: 0, color: '#fff' }}>
            Sentinel
          </Title>
        </div>
        <Menu
          mode="inline"
          theme="dark"
          selectedKeys={[location.pathname]}
          items={menuItems}
          onClick={({ key }) => navigate(key)}
          style={{ background: 'transparent', borderRight: 'none' }}
        />
      </Sider>
      <Layout>
        <Header
          style={{
            background: '#1a1a1a',
            borderBottom: '1px solid #303030',
            padding: '0 24px',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'space-between',
          }}
        >
          <Typography.Text style={{ color: '#888' }}>
            Sentinel SIEM v0.1.0
          </Typography.Text>
        </Header>
        <Content
          style={{
            margin: '16px',
            background: '#0a0a0a',
            borderRadius: 4,
            minHeight: 360,
          }}
        >
          <Routes>
            <Route path="/" element={<Navigate to="/posture" replace />} />
            <Route path="/posture" element={<SecurityPosture />} />
            <Route path="/search" element={<Search />} />
            <Route path="/incidents" element={<IncidentReview />} />
            <Route path="/threat-intel" element={<ThreatIntelligence />} />
            <Route path="/assets" element={<AssetCenter />} />
            <Route path="/risk" element={<RiskAnalysis />} />
            <Route path="/settings" element={<Settings />} />
          </Routes>
        </Content>
      </Layout>
    </Layout>
  );
};

export default App;
